#include "vm.h"


ASM_BEGIN

namespace 
  {

  bool is_8_bit(uint64_t number)
    {
    int8_t b = int8_t(number);
    return int64_t(b) == (int64_t)number;
    }

  bool is_16_bit(uint64_t number)
    {
    int16_t b = int16_t(number);
    return int64_t(b) == (int64_t)number;
    }

  bool is_32_bit(uint64_t number)
    {
    int32_t b = int32_t(number);
    return int64_t(b) == (int64_t)number;
    }

  /*
  byte 1: operation opcode: equal to (int)asmcode::operation value of instr.oper
  byte 2: first operand: equal to (int)asmcode::operand of instr.operand1
  byte 3: second operand: equal to (int)asmcode::operand of instr.operand2
  byte 4: information on operand1_mem and operand2_mem
          First four bits equal to: 0 => instr.operand1_mem equals zero
                                  : 1 => instr.operand1_mem needs 8 bits
                                  : 2 => instr.operand1_mem needs 16 bits
                                  : 3 => instr.operand1_mem needs 32 bits
                                  : 4 => instr.operand1_mem needs 64 bits
          Last four bits equal to : 0 => instr.operand2_mem equals zero
                                  : 1 => instr.operand2_mem needs 8 bits
                                  : 2 => instr.operand2_mem needs 16 bits
                                  : 3 => instr.operand2_mem needs 32 bits
                                  : 4 => instr.operand2_mem needs 64 bits
  byte 5+: instr.operand1_mem using as many bytes as warranted by byte 4, followed by instr.operand2_mem using as many bytes as warranted by byte4.
  */
  uint64_t fill_vm_bytecode(const asmcode::instruction& instr, uint8_t* opcode_stream)
    {  
    opcode_stream[0] = (uint8_t)instr.oper;
    opcode_stream[1] = (uint8_t)instr.operand1;
    opcode_stream[2] = (uint8_t)instr.operand2;
    uint8_t op1mem = 0;
    uint8_t op2mem = 0;
    if (instr.operand1_mem != 0)
      {
      if (is_8_bit(instr.operand1_mem))
        op1mem = 1;
      else if (is_16_bit(instr.operand1_mem))
        op1mem = 2;
      else if (is_32_bit(instr.operand1_mem))
        op1mem = 3;
      else
        op1mem = 4;
      }
    if (instr.operand2_mem != 0)
      {
      if (is_8_bit(instr.operand2_mem))
        op2mem = 1;
      else if (is_16_bit(instr.operand2_mem))
        op2mem = 2;
      else if (is_32_bit(instr.operand2_mem))
        op2mem = 3;
      else
        op2mem = 4;
      }
    opcode_stream[3] = (uint8_t)(op2mem << 4) | op1mem;
    uint64_t sz = 4;
    switch (op1mem)
      {
      case 1: opcode_stream[sz++] = (uint8_t)instr.operand1_mem; break;
      case 2: *(reinterpret_cast<uint16_t*>(opcode_stream + sz)) = (uint16_t)instr.operand1_mem; sz += 2; break;
      case 3: *(reinterpret_cast<uint32_t*>(opcode_stream + sz)) = (uint32_t)instr.operand1_mem; sz += 4; break;
      case 4: *(reinterpret_cast<uint64_t*>(opcode_stream + sz)) = (uint64_t)instr.operand1_mem; sz += 8; break;
      default: break;
      }
    switch (op2mem)
      {
      case 1: opcode_stream[sz++] = (uint8_t)instr.operand2_mem; break;
      case 2: *(reinterpret_cast<uint16_t*>(opcode_stream + sz)) = (uint16_t)instr.operand2_mem; sz += 2; break;
      case 3: *(reinterpret_cast<uint32_t*>(opcode_stream + sz)) = (uint32_t)instr.operand2_mem; sz += 4; break;
      case 4: *(reinterpret_cast<uint64_t*>(opcode_stream + sz)) = (uint64_t)instr.operand2_mem; sz += 8; break;
      default: break;
      }
    return sz;
    }


  void first_pass(first_pass_data& data, asmcode& code, const std::map<std::string, uint64_t>& externals)
    {
    uint8_t buffer[255];
    data.size = 0;
    data.label_to_address.clear();
    for (auto it = code.get_instructions_list().begin(); it != code.get_instructions_list().end(); ++it)
      {
      std::vector<std::pair<size_t, int>> nops_to_add;
      for (size_t i = 0; i < it->size(); ++i)
        {
        auto instr = (*it)[i];
        switch (instr.oper)
          {
          case asmcode::CALL:
          {
          auto it2 = externals.find(instr.text);
          if (it2 != externals.end())
            {
            instr.oper = asmcode::MOV;
            instr.operand1 = asmcode::RAX;
            instr.operand2 = asmcode::NUMBER;
            instr.operand2_mem = it2->second;
            data.size += fill_vm_bytecode(instr, buffer);
            instr.oper = asmcode::CALL;
            instr.operand1 = asmcode::RAX;
            instr.operand2 = asmcode::EMPTY;
            data.size += fill_vm_bytecode(instr, buffer);
            }
          else
            {
            if (instr.operand1 == asmcode::EMPTY)
              {
              instr.operand1 = asmcode::NUMBER;
              instr.operand1_mem = 0x11111111;
              }
            data.size += fill_vm_bytecode(instr, buffer);
            }
          break;
          }
          case asmcode::JMP:
          case asmcode::JE:
          case asmcode::JL:
          case asmcode::JLE:
          case asmcode::JG:
          case asmcode::JA:
          case asmcode::JB:
          case asmcode::JGE:
          case asmcode::JNE:
          {
          if (instr.operand1 == asmcode::EMPTY)
            {
            instr.operand1 = asmcode::NUMBER;
            instr.operand1_mem = 0x11111111;
            }
          data.size += fill_vm_bytecode(instr, buffer);
          break;
          }
          case asmcode::JMPS:
          case asmcode::JES:
          case asmcode::JLS:
          case asmcode::JLES:
          case asmcode::JGS:
          case asmcode::JAS:
          case asmcode::JBS:
          case asmcode::JGES:
          case asmcode::JNES:
          {
          instr.operand1 = asmcode::NUMBER;
          instr.operand1_mem = 0x11;
          data.size += fill_vm_bytecode(instr, buffer);
          break;
          }
          case asmcode::LABEL:
            data.label_to_address[instr.text] = data.size; break;
          case asmcode::LABEL_ALIGNED:
            if (data.size & 7)
              {
              int nr_of_nops = 8 - (data.size & 7);
              data.size += nr_of_nops;
              nops_to_add.emplace_back(i, nr_of_nops);
              }
            data.label_to_address[instr.text] = data.size; break;
          case asmcode::GLOBAL:
            if (data.size & 7)
              {
              int nr_of_nops = 8 - (data.size & 7);
              data.size += nr_of_nops;
              nops_to_add.emplace_back(i, nr_of_nops);
              }
            data.label_to_address[instr.text] = data.size; break;
          case asmcode::EXTERN:
          {
          auto it2 = externals.find(instr.text);
          if (it2 == externals.end())
            throw std::logic_error("error: external is not defined");
          data.external_to_address[instr.text] = it2->second;
          break;
          }
          default:
            data.size += fill_vm_bytecode(instr, buffer); break;
          }
        }
      size_t nops_offset = 0;
      for (auto nops : nops_to_add)
        {
        std::vector<asmcode::instruction> nops_instructions(nops.second, asmcode::instruction(asmcode::NOP));
        it->insert(it->begin() + nops_offset + nops.first, nops_instructions.begin(), nops_instructions.end());
        nops_offset += nops.second;
        }
      }
    }


  uint8_t* second_pass(uint8_t* func, const first_pass_data& data, const asmcode& code)
    {
    uint64_t address_start = (uint64_t)(reinterpret_cast<uint64_t*>(func));
    uint8_t* start = func;
    for (auto it = code.get_instructions_list().begin(); it != code.get_instructions_list().end(); ++it)
      {
      for (asmcode::instruction instr : *it)
        {
        if (instr.operand1 == asmcode::LABELADDRESS)
          {
          auto it2 = data.label_to_address.find(instr.text);
          if (it2 == data.label_to_address.end())
            throw std::logic_error("error: label is not defined");
          instr.operand1_mem = address_start + it2->second;
          }
        if (instr.operand2 == asmcode::LABELADDRESS)
          {
          auto it2 = data.label_to_address.find(instr.text);
          if (it2 == data.label_to_address.end())
            throw std::logic_error("error: label is not defined");
          instr.operand2_mem = address_start + it2->second;
          }
        switch (instr.oper)
          {
          case asmcode::CALL:
          {
          if (instr.operand1 != asmcode::EMPTY)
            break;
          auto it_label = data.label_to_address.find(instr.text);
          auto it_external = data.external_to_address.find(instr.text);
          if (it_external != data.external_to_address.end())
            {
            asmcode::instruction extra_instr(asmcode::MOV, asmcode::RAX, asmcode::NUMBER, it_external->second);
            func += fill_vm_bytecode(extra_instr, func);
            instr.operand1 = asmcode::RAX;
            }
          else if (it_label != data.label_to_address.end())
            {
            int64_t address = (int64_t)it_label->second;
            int64_t current = (int64_t)(func - start);
            instr.operand1 = asmcode::NUMBER;
            instr.operand1_mem = (int64_t(address - current - 5));
            }
          else
            throw std::logic_error("second_pass error: call target does not exist");
          break;
          }
          case asmcode::JE:
          case asmcode::JL:
          case asmcode::JLE:
          case asmcode::JG:
          case asmcode::JA:
          case asmcode::JB:
          case asmcode::JGE:
          case asmcode::JNE:
          {
          if (instr.operand1 != asmcode::EMPTY)
            break;
          auto it2 = data.label_to_address.find(instr.text);
          if (it2 == data.label_to_address.end())
            throw std::logic_error("second_pass error: label does not exist");
          int64_t address = (int64_t)it2->second;
          int64_t current = (int64_t)(func - start);
          instr.operand1 = asmcode::NUMBER;
          instr.operand1_mem = (int64_t(address - current - 6));
          break;
          }
          case asmcode::JMP:
          {
          if (instr.operand1 != asmcode::EMPTY)
            break;
          auto it2 = data.label_to_address.find(instr.text);
          if (it2 == data.label_to_address.end())
            throw std::logic_error("second_pass error: label does not exist");
          int64_t address = (int64_t)it2->second;
          int64_t current = (int64_t)(func - start);
          instr.operand1 = asmcode::NUMBER;
          instr.operand1_mem = (int64_t(address - current - 5));
          break;
          }
          case asmcode::JES:
          case asmcode::JLS:
          case asmcode::JLES:
          case asmcode::JGS:
          case asmcode::JAS:
          case asmcode::JBS:
          case asmcode::JGES:
          case asmcode::JNES:
          case asmcode::JMPS:
          {
          if (instr.operand1 != asmcode::EMPTY)
            break;
          auto it2 = data.label_to_address.find(instr.text);
          if (it2 == data.label_to_address.end())
            throw std::logic_error("second_pass error: label does not exist");
          int64_t address = (int64_t)it2->second;
          int64_t current = (int64_t)(func - start);
          instr.operand1 = asmcode::NUMBER;
          instr.operand1_mem = (int64_t(address - current - 2));
          if ((int64_t)instr.operand1_mem > 127 || (int64_t)instr.operand1_mem < -128)
            throw std::logic_error("second_pass error: jump short is too far");
          break;
          }
          }
        func += fill_vm_bytecode(instr, func);
        }
      }
    while ((uint64_t)(func - start) < data.size)
      {
      asmcode::instruction dummy(asmcode::NOP);
      func += fill_vm_bytecode(dummy, func);
      }
    return func;
    }
  }  

void* vm_bytecode(uint64_t& size, first_pass_data& d, asmcode& code, const std::map<std::string, uint64_t>& externals)
  {
  d.external_to_address.clear();
  d.label_to_address.clear();
  first_pass(d, code, externals);

  uint8_t* compiled_func = new uint8_t[d.size + d.data_size];

  if (!compiled_func)
    throw std::runtime_error("Could not allocate virtual memory");

  uint8_t* func_end = second_pass((uint8_t*)compiled_func, d, code);

  uint64_t size_used = func_end - (uint8_t*)compiled_func;

  if (size_used != d.size)
    {
    throw std::logic_error("error: error in size computation.");
    }

  size = d.size + d.data_size;

  return (void*)compiled_func;
  }

void* vm_bytecode(uint64_t& size, asmcode& code, const std::map<std::string, uint64_t>& externals)
  {
  first_pass_data d;
  return vm_bytecode(size, d, code, externals);
  }

void* vm_bytecode(uint64_t& size, first_pass_data& d, asmcode& code)
  {
  std::map<std::string, uint64_t> externals;
  return vm_bytecode(size, d, code, externals);
  }

void* vm_bytecode(uint64_t& size, asmcode& code)
  {
  std::map<std::string, uint64_t> externals;
  return vm_bytecode(size, code, externals);
  }

void free_bytecode(void* f, uint64_t size)
  {
  (void*)size;
  delete[] f;
  }

ASM_END