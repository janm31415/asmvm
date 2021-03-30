///////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////////

#include "VMTests.h"
#include <stdint.h>
#include "asm/asmcode.h"
#include "asm/vm.h"
#include "test_assert.h"

ASM_BEGIN

namespace
  {
  void test_vm_mov()
    {
    asmcode code;
    code.add(asmcode::MOV, asmcode::RAX, asmcode::RCX);

    uint64_t size;
    uint8_t* f = (uint8_t*)vm_bytecode(size, code);
    TEST_EQ(4, size);
    TEST_EQ((int)asmcode::MOV, (int)f[0]);
    TEST_EQ((int)asmcode::RAX, (int)f[1]);
    TEST_EQ((int)asmcode::RCX, (int)f[2]);
    TEST_EQ(0, (int)f[3]);
    free_bytecode(f, size);
    }
  }


ASM_END

void run_all_vm_tests()
  {
  using namespace ASM;
  test_vm_mov();
  }