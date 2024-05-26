#include "Programs.h"

#include "../VulkanObjs.h"

#include "../Vulkan/VulkanShaderModule.h"

#include <Accela/Render/IVulkanCalls.h>

#include <format>
#include <set>
#include <algorithm>

namespace Accela::Render
{

Programs::Programs(Common::ILogger::Ptr logger,
                   VulkanObjsPtr vulkan,
                   IShadersPtr shaders)
    : m_logger(std::move(logger))
    , m_vulkan(std::move(vulkan))
    , m_shaders(std::move(shaders))
{

}

void Programs::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Programs: Destroying all programs");

    while (!m_programDefs.empty())
    {
        DestroyProgram(m_programDefs.begin()->first);
    }
}

bool Programs::CreateProgram(const std::string& programName, const std::vector<std::string>& shaders)
{
    m_logger->Log(Common::LogLevel::Info, "Programs: Creating program: {}", programName);

    const auto it = m_programDefs.find(programName);
    if (it != m_programDefs.cend())
    {
        m_logger->Log(Common::LogLevel::Warning, "CreateProgram: Program already existed: {}", programName);
        return true;
    }

    const auto shaderModulesOpt = GetShaderModules(shaders);
    if (!shaderModulesOpt.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateProgram: Unable to process program as all shaders don't exist: {}", programName);
        return false;
    }

    const auto descriptorSetLayoutsOpt = GenerateDescriptorSetLayouts(*shaderModulesOpt, programName);
    if (!descriptorSetLayoutsOpt.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateProgram: Unable to process program as descriptor set layouts couldn't be created: {}", programName);
        return false;
    }

    const auto vertexInputDescriptions = GenerateVertexInputDescriptions(*shaderModulesOpt);
    if (!vertexInputDescriptions.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateProgram: Unable to process program as input descriptions couldn't be created: {}", programName);
        return false;
    }

    const auto programDef = std::make_shared<ProgramDef>(
        programName,
        shaders,
        *descriptorSetLayoutsOpt,
        vertexInputDescriptions->first,
        vertexInputDescriptions->second);

    m_programDefs.insert(std::make_pair(programName, programDef));

    return true;
}

ProgramDefPtr Programs::GetProgramDef(const std::string& programName) const
{
    const auto it = m_programDefs.find(programName);
    if (it != m_programDefs.cend())
    {
        return it->second;
    }

    return nullptr;
}

std::optional<std::vector<VulkanShaderModulePtr>>
Programs::GetShaderModules(const std::vector<std::string>& shaderFileNames) const
{
    std::vector<VulkanShaderModulePtr> shaderModules;

    for (const auto& shaderFileName : shaderFileNames)
    {
        const auto shaderModule = m_shaders->GetShaderModule(shaderFileName);
        if (!shaderModule)
        {
            m_logger->Log(Common::LogLevel::Error, "GetShaderModules: Shader not found: {}", shaderFileName);
            return std::nullopt;
        }

        shaderModules.push_back(*shaderModule);
    }

    return shaderModules;
}

std::optional<std::vector<VulkanDescriptorSetLayoutPtr>>
Programs::GenerateDescriptorSetLayouts(const std::vector<VulkanShaderModulePtr>& shaderModules, const std::string& tag) const
{
    //
    // Compile the set of unique descriptor set indices that exist across all the shader modules
    //
    std::set<uint32_t, std::less<>> uniqueDescriptorSets; // Sorted in ascending order 0..x

    for (const auto& module : shaderModules)
    {
        SpvReflectShaderModule reflectInfo = module->GetReflectInfo();

        for (uint32_t x = 0; x < reflectInfo.descriptor_set_count; ++x)
        {
            uniqueDescriptorSets.insert(reflectInfo.descriptor_sets[x].set);
        }
    }

    //
    // All shaders use up to 4 descriptor sets. Create a descriptor set layout which represents
    // the shaders' usage of each set. If the combination of shaders doesn't make use of a given
    // set, a mock/stub descriptor set layout is created instead.
    //
    std::vector<VulkanDescriptorSetLayoutPtr> layouts;

    for (unsigned int set = 0; set < 4; ++set)
    {
        VulkanDescriptorSetLayoutPtr layout;

        const auto it = uniqueDescriptorSets.find(set);

        if (it != uniqueDescriptorSets.cend())
        {
            // Create a descriptor set layout from the shader's usage of this set
            layout = GenerateDescriptorSetLayout(shaderModules, set, std::format("{}-{}", tag, set));
        }
        else
        {
            // None of the shaders uses this descriptor set index - create a stub descriptor set layout with
            // no bindings in order to make sure there's no gaps in-between sets in the pipeline config.
            layout = std::make_shared<VulkanDescriptorSetLayout>(m_logger, m_vulkan->GetCalls(), m_vulkan->GetDevice());
            layout->Create({}, std::format("{}-stub-{}", tag, set));
        }

        if (layout == nullptr)
        {
            for (auto& toDelete : layouts) { toDelete->Destroy(); }
            return std::nullopt;
        }

        layouts.push_back(layout);
    }

    return layouts;
}

VulkanDescriptorSetLayoutPtr
Programs::GenerateDescriptorSetLayout(const std::vector<VulkanShaderModulePtr>& shaderModules, uint32_t set, const std::string& tag) const
{
    // Map of descriptor set binding index to the spv reflection details of that binding index
    std::unordered_map<uint32_t, SpvReflectDescriptorBinding> setBindingReflectInfos;

    // Records which shader module stages include this descriptor set
    VkShaderStageFlags moduleSetUsagesFlags{0};

    //
    // Loop through the modules and compile information about how they use the descriptor set
    //
    for (const auto& module : shaderModules)
    {
        // Get the reflection info of this module's usage of the descriptor set, if any
        const auto reflectDescriptorSetOpt = GetModuleReflectDescriptorSet(module->GetReflectInfo(), set);
        if (reflectDescriptorSetOpt.has_value())
        {
            // Mark this module as using this descriptor set
            moduleSetUsagesFlags = moduleSetUsagesFlags | SpvToVkShaderStageFlags(module->GetReflectInfo().shader_stage).value();

            // Save the detail's of the descriptor set's bindings for later usage. Note that we're assuming
            // that any module that uses this descriptor set is required to use all the same bindings as
            // other modules.
            for (uint32_t x = 0; x < reflectDescriptorSetOpt->binding_count; ++x)
            {
                const auto setBinding = reflectDescriptorSetOpt->bindings[x];

                const auto it = setBindingReflectInfos.find(setBinding->binding);
                if (it == setBindingReflectInfos.cend())
                {
                    setBindingReflectInfos.insert(std::make_pair(setBinding->binding, *setBinding));
                }
            }
        }
    }

    //
    // Generate details about the descriptor set's bindings
    //
    std::vector<VulkanDescriptorSetLayout::BindingDetails> bindingDetails;

    std::transform(setBindingReflectInfos.begin(), setBindingReflectInfos.end(), std::back_inserter(bindingDetails), [&](const auto& spvBindingInfo) {
        VulkanDescriptorSetLayout::BindingDetails bindingInfo{};
        bindingInfo.descriptorSet   = spvBindingInfo.second.set;
        bindingInfo.binding         = spvBindingInfo.second.binding;
        bindingInfo.name            = spvBindingInfo.second.name;
        bindingInfo.descriptorType  = SpvToVkDescriptorType(spvBindingInfo.second.descriptor_type).value();
        bindingInfo.stageFlags      = moduleSetUsagesFlags;
        bindingInfo.descriptorCount = spvBindingInfo.second.count;

        return bindingInfo;
    });

    VulkanDescriptorSetLayoutPtr layout = std::make_shared<VulkanDescriptorSetLayout>(m_logger, m_vulkan->GetCalls(), m_vulkan->GetDevice());
    if (!layout->Create(bindingDetails, tag))
    {
        m_logger->Log(Common::LogLevel::Error, "GenerateDescriptorSetLayout: Failure creating descriptor set layout");
        return nullptr;
    }

    return layout;
}

std::optional<SpvReflectDescriptorSet>
Programs::GetModuleReflectDescriptorSet(const SpvReflectShaderModule& module, uint32_t set)
{
    for (uint32_t x = 0; x < module.descriptor_set_count; ++x)
    {
        if (module.descriptor_sets[x].set == set)
        {
            return module.descriptor_sets[x];
        }
    }

    return std::nullopt;
}

std::optional<std::pair<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>>
Programs::GenerateVertexInputDescriptions(const std::vector<VulkanShaderModulePtr>& shaderModules)
{
    for (const auto& module : shaderModules)
    {
        const auto vertexInputAttributeDescriptions = GetModuleVertexInputDescriptions(module->GetReflectInfo());
        if (vertexInputAttributeDescriptions.has_value())
        {
            return *vertexInputAttributeDescriptions;
        }
    }

    return std::nullopt;
}

std::optional<std::pair<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>>
Programs::GetModuleVertexInputDescriptions(const SpvReflectShaderModule& module)
{
    // Only look at vertex shaders for input attributes
    if (module.shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
    {
        return std::nullopt;
    }

    uint32_t count = 0;
    spvReflectEnumerateInputVariables(&module, &count, nullptr);

    std::vector<SpvReflectInterfaceVariable*> input_vars(count);
    spvReflectEnumerateInputVariables(&module, &count, input_vars.data());

    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = 0;  // computed below
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    attribute_descriptions.reserve(input_vars.size());

    for (const auto& input_var : input_vars)
    {
        const SpvReflectInterfaceVariable& refl_var = *input_var;

        // Skip over builtin variables like gl_InstanceId
        if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
        {
            continue;
        }

        VkVertexInputAttributeDescription attr_desc{};
        attr_desc.location = refl_var.location;
        attr_desc.binding = binding_description.binding;
        attr_desc.format = static_cast<VkFormat>(refl_var.format);
        attr_desc.offset = 0;  // final offset computed below after sorting.
        attribute_descriptions.push_back(attr_desc);
    }

    // Sort attributes by location
    std::sort(std::begin(attribute_descriptions), std::end(attribute_descriptions),
        [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
        return a.location < b.location;
    });

    // Compute final offsets of each attribute, and total vertex stride.

    for (auto& attribute : attribute_descriptions)
    {
        attribute.offset = binding_description.stride;
        binding_description.stride += FormatSize(attribute.format);
    }

    return std::make_pair(attribute_descriptions, binding_description);
}

void Programs::DestroyProgram(const std::string& programName)
{
    m_logger->Log(Common::LogLevel::Info, "Programs: Destroying program: {} ", programName);

    const auto it = m_programDefs.find(programName);
    if (it == m_programDefs.cend())
    {
        return;
    }

    for (const auto& descriptorSetLayout : it->second->GetDescriptorSetLayouts())
    {
        descriptorSetLayout->Destroy();
    }

    m_programDefs.erase(it);
}

uint32_t Programs::FormatSize(VkFormat format)
{
    uint32_t result = 0;
    switch (format) {
        case VK_FORMAT_UNDEFINED: result = 0; break;
        case VK_FORMAT_R4G4_UNORM_PACK8: result = 1; break;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_R5G6B5_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_B5G6R5_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16: result = 2; break;
        case VK_FORMAT_R8_UNORM: result = 1; break;
        case VK_FORMAT_R8_SNORM: result = 1; break;
        case VK_FORMAT_R8_USCALED: result = 1; break;
        case VK_FORMAT_R8_SSCALED: result = 1; break;
        case VK_FORMAT_R8_UINT: result = 1; break;
        case VK_FORMAT_R8_SINT: result = 1; break;
        case VK_FORMAT_R8_SRGB: result = 1; break;
        case VK_FORMAT_R8G8_UNORM: result = 2; break;
        case VK_FORMAT_R8G8_SNORM: result = 2; break;
        case VK_FORMAT_R8G8_USCALED: result = 2; break;
        case VK_FORMAT_R8G8_SSCALED: result = 2; break;
        case VK_FORMAT_R8G8_UINT: result = 2; break;
        case VK_FORMAT_R8G8_SINT: result = 2; break;
        case VK_FORMAT_R8G8_SRGB: result = 2; break;
        case VK_FORMAT_R8G8B8_UNORM: result = 3; break;
        case VK_FORMAT_R8G8B8_SNORM: result = 3; break;
        case VK_FORMAT_R8G8B8_USCALED: result = 3; break;
        case VK_FORMAT_R8G8B8_SSCALED: result = 3; break;
        case VK_FORMAT_R8G8B8_UINT: result = 3; break;
        case VK_FORMAT_R8G8B8_SINT: result = 3; break;
        case VK_FORMAT_R8G8B8_SRGB: result = 3; break;
        case VK_FORMAT_B8G8R8_UNORM: result = 3; break;
        case VK_FORMAT_B8G8R8_SNORM: result = 3; break;
        case VK_FORMAT_B8G8R8_USCALED: result = 3; break;
        case VK_FORMAT_B8G8R8_SSCALED: result = 3; break;
        case VK_FORMAT_B8G8R8_UINT: result = 3; break;
        case VK_FORMAT_B8G8R8_SINT: result = 3; break;
        case VK_FORMAT_B8G8R8_SRGB: result = 3; break;
        case VK_FORMAT_R8G8B8A8_UNORM: result = 4; break;
        case VK_FORMAT_R8G8B8A8_SNORM: result = 4; break;
        case VK_FORMAT_R8G8B8A8_USCALED: result = 4; break;
        case VK_FORMAT_R8G8B8A8_SSCALED: result = 4; break;
        case VK_FORMAT_R8G8B8A8_UINT: result = 4; break;
        case VK_FORMAT_R8G8B8A8_SINT: result = 4; break;
        case VK_FORMAT_R8G8B8A8_SRGB: result = 4; break;
        case VK_FORMAT_B8G8R8A8_UNORM: result = 4; break;
        case VK_FORMAT_B8G8R8A8_SNORM: result = 4; break;
        case VK_FORMAT_B8G8R8A8_USCALED: result = 4; break;
        case VK_FORMAT_B8G8R8A8_SSCALED: result = 4; break;
        case VK_FORMAT_B8G8R8A8_UINT: result = 4; break;
        case VK_FORMAT_B8G8R8A8_SINT: result = 4; break;
        case VK_FORMAT_B8G8R8A8_SRGB: result = 4; break;
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32: result = 4; break;
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32: result = 4; break;
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32: result = 4; break;
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: result = 4; break;
        case VK_FORMAT_A8B8G8R8_UINT_PACK32: result = 4; break;
        case VK_FORMAT_A8B8G8R8_SINT_PACK32: result = 4; break;
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32: result = 4; break;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32: result = 4; break;
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32: result = 4; break;
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32: result = 4; break;
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: result = 4; break;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32: result = 4; break;
        case VK_FORMAT_A2R10G10B10_SINT_PACK32: result = 4; break;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32: result = 4; break;
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32: result = 4; break;
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32: result = 4; break;
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: result = 4; break;
        case VK_FORMAT_A2B10G10R10_UINT_PACK32: result = 4; break;
        case VK_FORMAT_A2B10G10R10_SINT_PACK32: result = 4; break;
        case VK_FORMAT_R16_UNORM: result = 2; break;
        case VK_FORMAT_R16_SNORM: result = 2; break;
        case VK_FORMAT_R16_USCALED: result = 2; break;
        case VK_FORMAT_R16_SSCALED: result = 2; break;
        case VK_FORMAT_R16_UINT: result = 2; break;
        case VK_FORMAT_R16_SINT: result = 2; break;
        case VK_FORMAT_R16_SFLOAT: result = 2; break;
        case VK_FORMAT_R16G16_UNORM: result = 4; break;
        case VK_FORMAT_R16G16_SNORM: result = 4; break;
        case VK_FORMAT_R16G16_USCALED: result = 4; break;
        case VK_FORMAT_R16G16_SSCALED: result = 4; break;
        case VK_FORMAT_R16G16_UINT: result = 4; break;
        case VK_FORMAT_R16G16_SINT: result = 4; break;
        case VK_FORMAT_R16G16_SFLOAT: result = 4; break;
        case VK_FORMAT_R16G16B16_UNORM: result = 6; break;
        case VK_FORMAT_R16G16B16_SNORM: result = 6; break;
        case VK_FORMAT_R16G16B16_USCALED: result = 6; break;
        case VK_FORMAT_R16G16B16_SSCALED: result = 6; break;
        case VK_FORMAT_R16G16B16_UINT: result = 6; break;
        case VK_FORMAT_R16G16B16_SINT: result = 6; break;
        case VK_FORMAT_R16G16B16_SFLOAT: result = 6; break;
        case VK_FORMAT_R16G16B16A16_UNORM: result = 8; break;
        case VK_FORMAT_R16G16B16A16_SNORM: result = 8; break;
        case VK_FORMAT_R16G16B16A16_USCALED: result = 8; break;
        case VK_FORMAT_R16G16B16A16_SSCALED: result = 8; break;
        case VK_FORMAT_R16G16B16A16_UINT: result = 8; break;
        case VK_FORMAT_R16G16B16A16_SINT: result = 8; break;
        case VK_FORMAT_R16G16B16A16_SFLOAT: result = 8; break;
        case VK_FORMAT_R32_UINT: result = 4; break;
        case VK_FORMAT_R32_SINT: result = 4; break;
        case VK_FORMAT_R32_SFLOAT: result = 4; break;
        case VK_FORMAT_R32G32_UINT: result = 8; break;
        case VK_FORMAT_R32G32_SINT: result = 8; break;
        case VK_FORMAT_R32G32_SFLOAT: result = 8; break;
        case VK_FORMAT_R32G32B32_UINT: result = 12; break;
        case VK_FORMAT_R32G32B32_SINT: result = 12; break;
        case VK_FORMAT_R32G32B32_SFLOAT: result = 12; break;
        case VK_FORMAT_R32G32B32A32_UINT: result = 16; break;
        case VK_FORMAT_R32G32B32A32_SINT: result = 16; break;
        case VK_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
        case VK_FORMAT_R64_UINT: result = 8; break;
        case VK_FORMAT_R64_SINT: result = 8; break;
        case VK_FORMAT_R64_SFLOAT: result = 8; break;
        case VK_FORMAT_R64G64_UINT: result = 16; break;
        case VK_FORMAT_R64G64_SINT: result = 16; break;
        case VK_FORMAT_R64G64_SFLOAT: result = 16; break;
        case VK_FORMAT_R64G64B64_UINT: result = 24; break;
        case VK_FORMAT_R64G64B64_SINT: result = 24; break;
        case VK_FORMAT_R64G64B64_SFLOAT: result = 24; break;
        case VK_FORMAT_R64G64B64A64_UINT: result = 32; break;
        case VK_FORMAT_R64G64B64A64_SINT: result = 32; break;
        case VK_FORMAT_R64G64B64A64_SFLOAT: result = 32; break;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32: result = 4; break;
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: result = 4; break;

        default:
            break;
    }
    return result;
}

}
