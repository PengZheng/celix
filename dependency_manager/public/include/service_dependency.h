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
 * service_dependency.h
 *
 *  \date       Aug 9, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_DEPENDENCY_H_
#define SERVICE_DEPENDENCY_H_

#include <celixbool.h>

#include "service_reference.h"
#include "service_tracker.h"

struct serviceDependency {
	char * interface;
	void (*added)(void * handle, service_reference_t reference, void *);
	void (*changed)(void * handle, service_reference_t reference, void *);
	void (*removed)(void * handle, service_reference_t reference, void *);
	void ** autoConfigureField;

	bool started;
	bool available;
	bool required;
	service_tracker_t tracker;
	SERVICE service;
	service_reference_t reference;
	bundle_context_t context;
	void * serviceInstance;
	char * trackedServiceName;
	char * trackedServiceFilter;
};

typedef struct serviceDependency * SERVICE_DEPENDENCY;

SERVICE_DEPENDENCY serviceDependency_create(bundle_context_t context);
void * serviceDependency_getService(SERVICE_DEPENDENCY dependency);

SERVICE_DEPENDENCY serviceDependency_setRequired(SERVICE_DEPENDENCY dependency, bool required);
SERVICE_DEPENDENCY serviceDependency_setService(SERVICE_DEPENDENCY dependency, char * serviceName, char * filter);
SERVICE_DEPENDENCY serviceDependency_setCallbacks(SERVICE_DEPENDENCY dependency, void (*added)(void * handle, service_reference_t reference, void *),
		void (*changed)(void * handle, service_reference_t reference, void *),
		void (*removed)(void * handle, service_reference_t reference, void *));
SERVICE_DEPENDENCY serviceDependency_setAutoConfigure(SERVICE_DEPENDENCY dependency, void ** field);

void serviceDependency_start(SERVICE_DEPENDENCY dependency, SERVICE service);
void serviceDependency_stop(SERVICE_DEPENDENCY dependency, SERVICE service);

void serviceDependency_invokeAdded(SERVICE_DEPENDENCY dependency);
void serviceDependency_invokeRemoved(SERVICE_DEPENDENCY dependency);


#endif /* SERVICE_DEPENDENCY_H_ */