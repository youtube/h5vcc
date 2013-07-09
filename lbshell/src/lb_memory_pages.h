/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LB_MEMORY_PAGES_H_
#define _LB_MEMORY_PAGES_H_

#include "lb_platform.h"

lb_virtual_mem_t lb_allocate_virtual_address(size_t size,
                                             size_t page_size);
void lb_free_virtual_address(lb_virtual_mem_t address);
int lb_allocate_physical_memory(size_t memory_size,
                                size_t page_size,
                                lb_physical_mem_t* mem_id);
void lb_free_physical_memory(lb_physical_mem_t physical_address);
// Map the physical memory into the given virtual address space
int lb_map_memory(lb_virtual_mem_t virtual_address,
                  lb_physical_mem_t physical_address);
void lb_unmap_memory(lb_virtual_mem_t virtual_address,
                     lb_physical_mem_t* physical_address);

size_t lb_get_total_system_memory();
size_t lb_get_unallocated_memory();

#endif // _LB_MEMORY_PAGES_H_
