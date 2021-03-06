/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file api_program.cpp
 * \brief Programs
 */

#include "CL/cl.h"
#include <core/program.h>
#include <core/context.h>

#include <iostream>
#include <cstdlib>

// Program Object APIs
cl_program
clCreateProgramWithSource(cl_context        context,
                          cl_uint           count,
                          const char **     strings,
                          const size_t *    lengths,
                          cl_int *          errcode_ret)
{
#ifdef DBG_API
  std::cerr << "clCreateProgramWithSource\n";
#endif
    cl_int dummy_errcode;

    if (!errcode_ret)
        errcode_ret = &dummy_errcode;

    if (!context->isA(Coal::Object::T_Context))
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return 0;
    }

    if (!count || !strings)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return 0;
    }

    Coal::Program *program = new Coal::Program(context);

    *errcode_ret = CL_SUCCESS;
    *errcode_ret = program->loadSources(count, strings, lengths);

    if (*errcode_ret != CL_SUCCESS)
    {
        delete program;
        return 0;
    }

    return (cl_program)program;
}

cl_program
clCreateProgramWithBinary(cl_context            context,
                          cl_uint               num_devices,
                          const cl_device_id *  device_list,
                          const size_t *        lengths,
                          const unsigned char **binaries,
                          cl_int *              binary_status,
                          cl_int *              errcode_ret)
{
#ifdef DBG_API
  std::cerr << "clCreateProgramWithBinary\n";
#endif
    cl_int dummy_errcode;

    if (!errcode_ret)
        errcode_ret = &dummy_errcode;

    if (!context->isA(Coal::Object::T_Context))
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return 0;
    }

    if (!num_devices || !device_list || !lengths || !binaries)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return 0;
    }

    // Check the devices for compliance
    cl_uint context_num_devices = 0;
    cl_device_id *context_devices;

    *errcode_ret = context->info(CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint),
                                 &context_num_devices, 0);

    if (*errcode_ret != CL_SUCCESS)
        return 0;

    context_devices =
        (cl_device_id *)std::malloc(context_num_devices * sizeof(cl_device_id));

    *errcode_ret = context->info(CL_CONTEXT_DEVICES,
                                 context_num_devices * sizeof(cl_device_id),
                                 context_devices, 0);

    if (*errcode_ret != CL_SUCCESS)
        return 0;

    for (cl_uint i=0; i<num_devices; ++i)
    {
        bool found = false;

        if (!lengths[i] || !binaries[i])
        {
            if (binary_status)
                binary_status[i] = CL_INVALID_VALUE;

            *errcode_ret = CL_INVALID_VALUE;
            return 0;
        }

        for (cl_uint j=0; j<context_num_devices; ++j)
        {
            if (device_list[i] == context_devices[j])
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            *errcode_ret = CL_INVALID_DEVICE;
            return 0;
        }
    }

    // Create a program
    Coal::Program *program = new Coal::Program(context);
    *errcode_ret = CL_SUCCESS;

    // Init program
    *errcode_ret = program->loadBinaries(binaries,
                                         lengths, binary_status, num_devices,
                                         (Coal::DeviceInterface * const*)device_list);

    if (*errcode_ret != CL_SUCCESS)
    {
        delete program;
        return 0;
    }

    return (cl_program)program;
}

cl_int
clRetainProgram(cl_program program)
{
    if (!program->isA(Coal::Object::T_Program))
        return CL_INVALID_PROGRAM;

    program->reference();

    return CL_SUCCESS;
}

cl_int
clReleaseProgram(cl_program program)
{
#ifdef DBG_API
  std::cerr << "Entering clReleaseProgram\n";
#endif
    if (!program->isA(Coal::Object::T_Program))
        return CL_INVALID_PROGRAM;

    if (program->dereference())
        delete program;
#ifdef DBG_API
    std::cerr << "Leaving clReleaseProgram\n";
#endif
    return CL_SUCCESS;
}

cl_int
clBuildProgram(cl_program           program,
               cl_uint              num_devices,
               const cl_device_id * device_list,
               const char *         options,
               void (*pfn_notify)(cl_program program, void * user_data),
               void *               user_data)
{
#ifdef DBG_API
  std::cerr << "Entering clBuildProgram. Building for " << num_devices
    << " devices\n";
#endif
  
    if (!program->isA(Coal::Object::T_Program)) {
      std::cerr << "INVALID_PROGRAM\n";
        return CL_INVALID_PROGRAM;
    }

    if (!device_list && num_devices > 0) {
      std::cerr << "!device_list : INVALID_VALUE\n";
        return CL_INVALID_VALUE;
    }

    if (!num_devices && device_list) {
      std::cerr << "!num_devices : INVALID_VALUE\n";
        return CL_INVALID_VALUE;
    }

    if (!pfn_notify && user_data) {
      std::cerr << "!pfn_notify : INVALID_VALUE\n";
        return CL_INVALID_VALUE;
    }
    // We cannot try to build a previously-failed program
    if (program->state() != Coal::Program::Loaded)
      return CL_INVALID_OPERATION;

#ifdef DBG_API
    std::cerr << "Checking " << num_devices << " devices\n";
#endif
    // Check the devices for compliance
    //if (num_devices)
    //{
        cl_uint context_num_devices = 0;
        cl_device_id *context_devices;
        Coal::Context *context = (Coal::Context *)program->parent();
        cl_int result;

        result = context->info(CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint),
                                     &context_num_devices, 0);

        if (result != CL_SUCCESS)
            return result;

        context_devices =
            (cl_device_id *)std::malloc(context_num_devices * sizeof(cl_device_id));

        result = context->info(CL_CONTEXT_DEVICES,
                                     context_num_devices * sizeof(cl_device_id),
                                     context_devices, 0);

        if (result != CL_SUCCESS) {
#ifdef DBG_API
          std::cerr << "context->info != CL_SUCCESS\n";
#endif
            return result;
        }

#ifdef DBG_API
        std::cerr << "Checking " << context_num_devices << " context devices\n";
#endif

        if (num_devices) {
          for (cl_uint i=0; i < num_devices; ++i) {
            bool found = false;
#ifdef DBG_API
            std::cerr << "device_list[" << i << "] addr = "
              << device_list[i] << std::endl;
#endif

            for (cl_uint j=0; j<context_num_devices; ++j)
            {
#ifdef DBG_API
              std::cerr << "context_device [" << j << "] addr = "
                << context_devices[j] << std::endl;
#endif
                if (device_list[i] == context_devices[j])
                {
                    found = true;
#ifdef DBG_API
                    std::cerr << "Found device, break out\n";
#endif
                    break;
                }
            }

            if (!found) {
              std::cerr << "INVALID_DEVICE\n";
              return CL_INVALID_DEVICE;
            }
        }
#ifdef DEBUCL
        std::cerr << "Leaving clBuildProgram after program->build\n";
#endif
        // Build program
        return program->build(options, pfn_notify, user_data, num_devices,
                              (Coal::DeviceInterface * const*)device_list);
      }
      // num devices wasn't specified and device_list is probably null, so
      // build for all associated devices
      else {
#ifdef DEBUCL
        std::cerr << "Leaving clBuildProgram after program->build\n";
#endif
        return program->build(options, pfn_notify, user_data,
                              context_num_devices,
                              context->getAllDevices());
      }
  return NULL;
}

cl_int
clUnloadCompiler(void)
{
    return CL_SUCCESS;
}

cl_int
clGetProgramInfo(cl_program         program,
                 cl_program_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret)
{
#ifdef DBG_API
  std::cerr << "clGetProgramInfo\n";
#endif

    if (!program->isA(Coal::Object::T_Program))
        return CL_INVALID_PROGRAM;

    return program->info(param_name, param_value_size, param_value,
                         param_value_size_ret);
}

cl_int
clGetProgramBuildInfo(cl_program            program,
                      cl_device_id          device,
                      cl_program_build_info param_name,
                      size_t                param_value_size,
                      void *                param_value,
                      size_t *              param_value_size_ret)
{
    if (!program->isA(Coal::Object::T_Program))
        return CL_INVALID_PROGRAM;

    return program->buildInfo((Coal::DeviceInterface *)device, param_name,
                              param_value_size, param_value,
                              param_value_size_ret);
}
