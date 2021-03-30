#pragma once

#include "namespace.h"
#include "asm_api.h"
#include "asmcode.h"
#include "assembler.h"

ASM_BEGIN

ASSEMBLER_API void* vm_bytecode(uint64_t& size, first_pass_data& d, asmcode& code, const std::map<std::string, uint64_t>& externals);
ASSEMBLER_API void* vm_bytecode(uint64_t& size, first_pass_data& d, asmcode& code);
ASSEMBLER_API void* vm_bytecode(uint64_t& size, asmcode& code, const std::map<std::string, uint64_t>& externals);
ASSEMBLER_API void* vm_bytecode(uint64_t& size, asmcode& code);

ASSEMBLER_API void free_bytecode(void* f, uint64_t size);

ASM_END