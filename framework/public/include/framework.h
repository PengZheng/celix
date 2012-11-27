/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * framework.h
 *
 *  \date       Mar 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

typedef struct activator * ACTIVATOR;
typedef struct framework * FRAMEWORK;

#include "manifest.h"
#include "wire.h"
#include "hash_map.h"
#include "array_list.h"
#include "celix_errno.h"
#include "service_factory.h"
#include "bundle_archive.h"
#include "service_listener.h"
#include "bundle_listener.h"
#include "service_registration.h"
#include "bundle_context.h"
#include "framework_exports.h"

FRAMEWORK_EXPORT celix_status_t framework_create(FRAMEWORK *framework, apr_pool_t *memoryPool, PROPERTIES config);
FRAMEWORK_EXPORT celix_status_t framework_destroy(FRAMEWORK framework);

FRAMEWORK_EXPORT celix_status_t fw_init(FRAMEWORK framework);
FRAMEWORK_EXPORT celix_status_t framework_start(FRAMEWORK framework);
FRAMEWORK_EXPORT void framework_stop(FRAMEWORK framework);

FRAMEWORK_EXPORT celix_status_t fw_getProperty(FRAMEWORK framework, const char *name, char **value);

FRAMEWORK_EXPORT celix_status_t fw_installBundle(FRAMEWORK framework, BUNDLE * bundle, char * location, char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_uninstallBundle(FRAMEWORK framework, BUNDLE bundle);

FRAMEWORK_EXPORT celix_status_t framework_getBundleEntry(FRAMEWORK framework, BUNDLE bundle, char *name, apr_pool_t *pool, char **entry);

FRAMEWORK_EXPORT celix_status_t fw_startBundle(FRAMEWORK framework, BUNDLE bundle, int options);
FRAMEWORK_EXPORT celix_status_t framework_updateBundle(FRAMEWORK framework, BUNDLE bundle, char *inputFile);
FRAMEWORK_EXPORT celix_status_t fw_stopBundle(FRAMEWORK framework, BUNDLE bundle, bool record);

FRAMEWORK_EXPORT celix_status_t fw_registerService(FRAMEWORK framework, SERVICE_REGISTRATION * registration, BUNDLE bundle, char * serviceName, void * svcObj, PROPERTIES properties);
FRAMEWORK_EXPORT celix_status_t fw_registerServiceFactory(FRAMEWORK framework, SERVICE_REGISTRATION * registration, BUNDLE bundle, char * serviceName, service_factory_t factory, PROPERTIES properties);
FRAMEWORK_EXPORT void fw_unregisterService(SERVICE_REGISTRATION registration);

FRAMEWORK_EXPORT celix_status_t fw_getServiceReferences(FRAMEWORK framework, ARRAY_LIST *references, BUNDLE bundle, const char * serviceName, char * filter);
FRAMEWORK_EXPORT void * fw_getService(FRAMEWORK framework, BUNDLE bundle, SERVICE_REFERENCE reference);
FRAMEWORK_EXPORT bool framework_ungetService(FRAMEWORK framework, BUNDLE bundle, SERVICE_REFERENCE reference);
FRAMEWORK_EXPORT celix_status_t fw_getBundleRegisteredServices(FRAMEWORK framework, apr_pool_t *pool, BUNDLE bundle, ARRAY_LIST *services);
FRAMEWORK_EXPORT celix_status_t fw_getBundleServicesInUse(FRAMEWORK framework, BUNDLE bundle, ARRAY_LIST *services);

FRAMEWORK_EXPORT void fw_addServiceListener(FRAMEWORK framework, BUNDLE bundle, SERVICE_LISTENER listener, char * filter);
FRAMEWORK_EXPORT void fw_removeServiceListener(FRAMEWORK framework, BUNDLE bundle, SERVICE_LISTENER listener);

FRAMEWORK_EXPORT celix_status_t fw_addBundleListener(FRAMEWORK framework, BUNDLE bundle, bundle_listener_t listener);
FRAMEWORK_EXPORT celix_status_t fw_removeBundleListener(FRAMEWORK framework, BUNDLE bundle, bundle_listener_t listener);

FRAMEWORK_EXPORT void fw_serviceChanged(FRAMEWORK framework, SERVICE_EVENT_TYPE eventType, SERVICE_REGISTRATION registration, PROPERTIES oldprops);

FRAMEWORK_EXPORT celix_status_t fw_isServiceAssignable(FRAMEWORK fw, BUNDLE requester, SERVICE_REFERENCE reference, bool *assignable);

//bundle_archive_t fw_createArchive(long id, char * location);
//void revise(bundle_archive_t archive, char * location);
FRAMEWORK_EXPORT celix_status_t getManifest(bundle_archive_t archive, apr_pool_t *pool, MANIFEST *manifest);

FRAMEWORK_EXPORT BUNDLE findBundle(bundle_context_t context);
FRAMEWORK_EXPORT SERVICE_REGISTRATION findRegistration(SERVICE_REFERENCE reference);

FRAMEWORK_EXPORT SERVICE_REFERENCE listToArray(ARRAY_LIST list);
FRAMEWORK_EXPORT celix_status_t framework_markResolvedModules(FRAMEWORK framework, HASH_MAP wires);

FRAMEWORK_EXPORT celix_status_t framework_waitForStop(FRAMEWORK framework);

FRAMEWORK_EXPORT ARRAY_LIST framework_getBundles(FRAMEWORK framework);
FRAMEWORK_EXPORT BUNDLE framework_getBundle(FRAMEWORK framework, char * location);
FRAMEWORK_EXPORT BUNDLE framework_getBundleById(FRAMEWORK framework, long id);

FRAMEWORK_EXPORT celix_status_t framework_getMemoryPool(FRAMEWORK framework, apr_pool_t **pool);
FRAMEWORK_EXPORT celix_status_t framework_getFrameworkBundle(FRAMEWORK framework, BUNDLE *bundle);

#endif /* FRAMEWORK_H_ */
