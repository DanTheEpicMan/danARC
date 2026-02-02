#include "memory.h"

#include <iostream>
#include <cstring>
#include <signal.h>
#include <vector>


// Global variables, needed for easy read without a class

pid_t ProcessId = 0;
long BaseAddress = 0;

#define debug false

// Memory read/write function templates implementation
template<typename T>
T ReadMemory(pid_t pid, long address)
#if !debug
{
    T buffer;
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = &buffer;
    local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = sizeof(T);

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nread != sizeof(T))
    {
        // Silent failure - don't spam console
        memset(&buffer, 0, sizeof(T));
    }

    return buffer;
}
#else
{
    T buffer;
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = &buffer;
    local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = sizeof(T);

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nread != sizeof(T))
    {
        std::cerr << "[ReadMemory] Failed to read " << sizeof(T)
                  << " bytes at address 0x" << std::hex << address
                  << " (pid " << pid << "). nread=" << nread << std::endl;
        perror("[ReadMemory] process_vm_readv");
        memset(&buffer, 0, sizeof(T));
    }

    return buffer;
}
#endif

// Implementation for GameData.h template
template<typename T>
T ReadMemory(uintptr_t address)
{
    return ReadMemory<T>(ProcessId, address);
}

void ReadMemoryBuffer(pid_t pid, long address, void* buffer, size_t size)
{

    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = size;

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nread != (ssize_t)size)
    {
        // If the read fails, zero out the buffer to prevent using old/garbage data.
        memset(buffer, 0, size);
#if debug
        std::cerr << "[ReadMemory (Buffer)] Failed to read " << size
                  << " bytes at address 0x" << std::hex << address
                  << " (pid " << pid << "). nread=" << nread << std::endl;
        perror("[ReadMemory (Buffer)] process_vm_readv");
        //std::cout << "Trace: " << __FILE__ << ":" << __LINE__ << " in " << __func__ << std::endl;
#endif
    }
}

void ReadMemoryBuffer(uintptr_t address, void* buffer, size_t size)
{
    ReadMemoryBuffer(ProcessId, address, buffer, size);
}

template<typename T>
bool WriteMemory(pid_t pid, long address, const T& value)
{
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = (void*)&value;
    local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = sizeof(T);

    ssize_t nwrite = process_vm_writev(pid, local, 1, remote, 1, 0);
    return (nwrite == sizeof(T));
}

// Read a string from memory with maximum length
std::string ReadString(pid_t pid, long address, size_t maxLength)
{
    std::vector<char> buffer(maxLength, 0);

    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer.data();
    local[0].iov_len = maxLength;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = maxLength;

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nread <= 0)
    {
        return "";
    }

    // Ensure null-termination
    buffer[maxLength - 1] = '\0';
    return std::string(buffer.data());
}


// Explicit template instantiations for common types
template int ReadMemory<int>(pid_t pid, long address);
template long ReadMemory<long>(pid_t pid, long address);
template float ReadMemory<float>(pid_t pid, long address);
template double ReadMemory<double>(pid_t pid, long address);
template uintptr_t ReadMemory<uintptr_t>(pid_t pid, long address);

template bool WriteMemory<int>(pid_t pid, long address, const int& value);
template bool WriteMemory<long>(pid_t pid, long address, const long& value);
template bool WriteMemory<float>(pid_t pid, long address, const float& value);
template bool WriteMemory<double>(pid_t pid, long address, const double& value);

// Explicit template instantiations for GameData.h template
template int ReadMemory<int>(uintptr_t address);
template long ReadMemory<long>(uintptr_t address);
template float ReadMemory<float>(uintptr_t address);
template double ReadMemory<double>(uintptr_t address);
template uintptr_t ReadMemory<uintptr_t>(uintptr_t address);
template Vector3 ReadMemory<Vector3>(uintptr_t address);
template Matrix4x4 ReadMemory<Matrix4x4>(uintptr_t address);
template FminimalViewInfo ReadMemory<FminimalViewInfo>(uintptr_t address);

//For reading player list or bone list and stuff
// template TArray<uintptr_t> ReadMemory<TArray<uintptr_t>>(uintptr_t address);
// template TArray<FTransform> ReadMemory<TArray<FTransform>>(uintptr_t address);
// //For Camera Stuff
// template FCameraCacheEntry ReadMemory<FCameraCacheEntry>(uintptr_t address);
// //For player location
// template FVector ReadMemory<FVector>(uintptr_t address);
// template FRotator ReadMemory<FRotator>(uintptr_t address);
// template FQuat ReadMemory<FQuat>(uintptr_t address);
// //For TRansform (bones, ship holes, etc.)
// template FTransform ReadMemory<FTransform>(uintptr_t address);
// template GUID ReadMemory<GUID>(uintptr_t address);
// template RepMovement ReadMemory<RepMovement>(uintptr_t address);

template char ReadMemory<char>(uintptr_t address);
template bool ReadMemory<bool>(uintptr_t address);