
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

// Include the kernel strings
#include "sweep_kernels.h"


// Global OpenCL handles (context, queue, etc.)
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;

// OpenCL kernels
cl_kernel k_sweep;


// Check OpenCL errors and exit if no success
void check_error(cl_int err, char *msg)
{
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "Error %d: %s\n", err, msg);
        exit(err);
    }
}

// Check for OpenCL build errors and display build messages
void check_build_error(cl_int err, char *msg)
{
    if (err == CL_BUILD_PROGRAM_FAILURE)
    {
        char *build_log = (char*)malloc(sizeof(char)*2048);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(char)*2048, build_log, NULL);
        fprintf(stderr, "Error: %d\n", err);
        fprintf(stderr, "Build log:\n%s\n", build_log);
        free(build_log);
    }
    check_error(err, msg);
}


void opencl_setup_(void)
{
    printf("Setting up OpenCL environment...");

    cl_int err;

    // Secure a platform
    cl_uint num_platforms;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    check_error(err, "Finding platforms");

    cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*num_platforms);
    
    err = clGetPlatformIDs(num_platforms, platforms, NULL);
    check_error(err, "Getting platforms");

    // Get a device
    cl_device_type type = CL_DEVICE_TYPE_GPU;
    cl_platform_id platform;
    for (unsigned int i = 0; i < num_platforms; i++)
    {
        cl_uint num_devices = 0;
        err = clGetDeviceIDs(platforms[i], type, 0, NULL, &num_devices);
        if (num_devices > 0 && err == CL_SUCCESS)
        {
            err = clGetDeviceIDs(platforms[i], type, 1, &device, NULL);
            platform = platforms[i];
            check_error(err, "Securing a device");
            break;
        }
    }
    check_error(err, "Could not find a device");

    // Create a context
    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
    context = clCreateContext(properties, 1, &device, NULL, NULL, &err);
    check_error(err, "Creating context");

    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    check_error(err, "Creating command queue");

    // Create program
    program = clCreateProgramWithSource(context, 1, &sweep_kernels_ocl, NULL, &err);
    check_error(err, "Creating program");

    // Build program
    char *options = "";
    err = clBuildProgram(program, 1, &device, options, NULL, NULL);
    check_build_error(err, "Building program");

    // Create kernels
    k_sweep = clCreateKernel(program, "sweep", &err);
    check_error(err, "Creating kernel sweep");

    free(platforms);
    printf("done\n");
}

// Release the global OpenCL handles
void opencl_teardown_(void)
{
    printf("Releasing OpenCL...");

    cl_int err;
    err = clReleaseDevice(device);
    check_error(err, "Releasing device");

    err = clReleaseContext(context);
    check_error(err, "Releasing context");

    err = clReleaseCommandQueue(queue);
    check_error(err, "Releasing queue");

    printf("done\n");
}

