/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include <mutex>

#include "celix/LogHelper.h"
#include "celix/rsa/EndpointDescription.h"
#include <celix/rsa/IImportServiceFactory.h>
#include <celix/rsa/IExportServiceFactory.h>

#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#if __cpp_lib_memory_resource
#include <memory_resource>
#endif

namespace celix::rsa {

    class RemoteServiceAdmin {
    public:
        explicit RemoteServiceAdmin(celix::LogHelper logHelper);

        // Imported endpoint add/remove functions
        void addEndpoint(const std::shared_ptr<celix::rsa::EndpointDescription>& endpoint);
        void removeEndpoint(const std::shared_ptr<celix::rsa::EndpointDescription>& endpoint);

        // import service factory. used to create new imported services
        void addImportedServiceFactory(const std::shared_ptr<celix::rsa::IImportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);
        void removeImportedServiceFactory(const std::shared_ptr<celix::rsa::IImportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);

        // export service factory. used to create new exported services
        void addExportedServiceFactory(const std::shared_ptr<celix::rsa::IExportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);
        void removeExportedServiceFactory(const std::shared_ptr<celix::rsa::IExportServiceFactory>& factory, const std::shared_ptr<const celix::Properties>& properties);

        // Add/remove services, used to track all services registered with celix and check if remote = true.
        void addService(const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties);
        void removeService(const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties);
    private:
        void createExportServices();
        void createImportServices();

        celix::LogHelper _logHelper;
        std::mutex _m{}; // protects below

#if __cpp_lib_memory_resource
        std::pmr::unsynchronized_pool_resource _memResource{};

        std::pmr::unordered_map<std::string, std::shared_ptr<celix::rsa::IExportServiceFactory>> _exportServiceFactories{&_memResource}; //key = service name
        std::pmr::unordered_map<std::string, std::shared_ptr<celix::rsa::IImportServiceFactory>> _importServiceFactories{&_memResource}; //key = service name
        std::pmr::unordered_map<std::string, std::unique_ptr<celix::rsa::IImportServiceGuard>> _importedServices{&_memResource}; //key = endpoint id
        std::pmr::unordered_map<long, std::unique_ptr<celix::rsa::IExportServiceGuard>> _exportedServices{&_memResource}; //key = service id
#else
        std::unordered_map<std::string, std::shared_ptr<celix::rsa::IExportServiceFactory>> _exportServiceFactories{}; //key = service name
        std::unordered_map<std::string, std::shared_ptr<celix::rsa::IImportServiceFactory>> _importServiceFactories{}; //key = service name
        std::unordered_map<std::string, std::unique_ptr<celix::rsa::IImportServiceGuard>> _importedServices{}; //key = endpoint id
        std::unordered_map<long, std::unique_ptr<celix::rsa::IExportServiceGuard>> _exportedServices{}; //key = service id
#endif
        std::vector<std::shared_ptr<celix::rsa::EndpointDescription>> _toBeImportedServices{};
        std::vector<std::shared_ptr<const celix::Properties>> _toBeExportedServices{};
    };
}