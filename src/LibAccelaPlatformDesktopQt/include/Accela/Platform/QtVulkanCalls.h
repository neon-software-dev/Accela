#ifndef LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANCALLS_H
#define LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANCALLS_H

#include "QtVulkanInstance.h"

#include <Accela/Render/VulkanCalls.h>

#include <QVulkanInstance>

namespace Accela::Platform
{
    /**
     * Uses SDL to retrieve the vkGetInstanceProcAddr function
     */
    class QtVulkanCalls : public Render::VulkanCalls
    {
        public:

            explicit QtVulkanCalls(QtVulkanInstance::Ptr qtVulkanInstance)
                : m_qtVulkanInstance(std::move(qtVulkanInstance))
            {

            }

            bool InitInstanceCalls(VkInstance vkInstance) override
            {
                //
                // Up until the Renderer create an instance, we were using a default
                // QVulkanInstance. Now that we're at the point where we're looking
                // up functions for a particular instance, notify QtVulkanInstance to
                // recreate its internal QVulkanInstance based on the VkInstance that
                // the renderer is providing.
                //
                if (!m_qtVulkanInstance->CreateFromVkInstance(vkInstance))
                {
                    return false;
                }

                // Technically not needed, since global funcs are never called again
                // after instance creation
                if (!Render::VulkanCalls::InitGlobalCalls())
                {
                    return false;
                }

                //
                // Continue with the normal instance calls lookup using the new QVulkanInstance
                //
                return Render::VulkanCalls::InitInstanceCalls(vkInstance);
            }

        protected:

            PFN_vkGetInstanceProcAddr GetInstanceProcAddrFunc() override
            {
                return (PFN_vkGetInstanceProcAddr)m_qtVulkanInstance->GetQVulkanInstance()
                    ->getInstanceProcAddr("vkGetInstanceProcAddr");
            }

        private:

            QtVulkanInstance::Ptr m_qtVulkanInstance;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTVULKANCALLS_H
