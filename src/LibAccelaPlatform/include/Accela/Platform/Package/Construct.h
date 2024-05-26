#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_CONSTRUCT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_CONSTRUCT_H

#include <string>

namespace Accela::Platform
{
    class Construct
    {
        public:

            explicit Construct(std::string name)
                : m_name(std::move(name))
            {

            }

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }

        private:

            std::string m_name;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_PACKAGE_CONSTRUCT_H
