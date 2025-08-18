#!/usr/bin/env python3
"""
Command Line Interface for MIGHF-V3LC Micro-Architecture
"""

import argparse
import sys
import json
import struct
from pathlib import Path
from typing import Optional

try:
    from .mighf_v3lc import (
        MIGHFV3LC, AssemblyParser, DisassemblerEngine, 
        SymbolicExecutor, BinaryParser, MIGHFException
    )
except ImportError:
    # Fallback for direct execution
    from mighf_v3lc import (
        MIGHFV3LC, AssemblyParser, DisassemblerEngine,
        SymbolicExecutor, BinaryParser, MIGHFException
    )


def setup_logging(verbose: bool = False):
    """Setup logging configuration"""
    import logging
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )


def assemble_main():
    """Main entry point for mighf-asm command"""
    parser = argparse.ArgumentParser(
        description='MIGHF-V3LC Assembler',
        prog='mighf-asm'
    )
    parser.add_argument('input', help='Input assembly file')
    parser.add_argument('-o', '--output', help='Output binary file')
    parser.add_argument('-f', '--format', choices=['binary', 'hex'], 
                       default='binary', help='Output format')
    parser.add_argument('-v', '--verbose', action='store_true', 
                       help='Verbose output')
    parser.add_argument('--base-address', type=lambda x: int(x, 0), 
                       default=0, help='Base address for assembly')
    parser.add_argument('--list', action='store_true', 
                       help='Generate assembly listing')
    
    args = parser.parse_args()
    setup_logging(args.verbose)
    
    try:
        # Read input assembly file
        with open(args.input, 'r') as f:
            assembly_code = f.read()
        
        # Parse and assemble
        assembler = AssemblyParser()
        instructions = assembler.parse_assembly(assembly_code)
        
        print(f"Assembled {len(instructions)} instructions")
        
        # Generate binary
        binary_code = b''
        for instruction in instructions:
            binary_code += instruction.encode()
        
        # Output file handling
        if args.output:
            output_file = args.output
        else:
            input_path = Path(args.input)
            output_file = input_path.with_suffix('.bin')
        
        # Write output
        if args.format == 'binary':
            with open(output_file, 'wb') as f:
                f.write(binary_code)
        else:  # hex format
            with open(output_file, 'w') as f:
                hex_output = binary_code.hex()
                # Format as Intel HEX or similar
                for i in range(0, len(hex_output), 32):
                    f.write(hex_output[i:i+32] + '\n')
        
        print(f"Output written to {output_file}")
        
        # Generate listing if requested
        if args.list:
            listing_file = Path(output_file).with_suffix('.lst')
            with open(listing_file, 'w') as f:
                f.write(f"; MIGHF-V3LC Assembly Listing\n")
                f.write(f"; Source: {args.input}\n")
                f.write(f"; Base Address: 0x{args.base_address:08x}\n\n")
                
                for i, instruction in enumerate(instructions):
                    addr = args.base_address + (i * 4)
                    code_bytes = instruction.encode()
                    hex_bytes = ' '.join(f'{b:02x}' for b in code_bytes)
                    
                    f.write(f"{addr:08x}: {hex_bytes:<12} {instruction.opcode.name}")
                    if instruction.operands:
                        f.write(f" {', '.join(str(op) for op in instruction.operands)}")
                    if instruction.immediate is not None:
                        f.write(f", #{instruction.immediate}")
                    f.write('\n')
            
            print(f"Listing written to {listing_file}")
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def disassemble_main():
    """Main entry point for mighf-disasm command"""
    parser = argparse.ArgumentParser(
        description='MIGHF-V3LC Disassembler',
        prog='mighf-disasm'
    )
    parser.add_argument('input', help='Input binary file')
    parser.add_argument('-o', '--output', help='Output assembly file')
    parser.add_argument('-f', '--format', choices=['mighf', 'x86', 'x64', 'arm'],
                       default='mighf', help='Input format')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    parser.add_argument('--base-address', type=lambda x: int(x, 0),
                       default=0, help='Base address for disassembly')
    parser.add_argument('--symbols', help='Symbol file for labeling')
    
    args = parser.parse_args()
    setup_logging(args.verbose)
    
    try:
        # Read input binary
        with open(args.input, 'rb') as f:
            binary_data = f.read()
        
        # Create disassembler
        disasm = DisassemblerEngine()
        
        # Disassemble based on format
        if args.format == 'mighf':
            instructions = disasm.disassemble_mighf(binary_data, args.base_address)
            output_lines = []
            
            for instruction in instructions:
                line = f"{instruction.opcode.name}"
                if instruction.operands:
                    line += f" {', '.join(str(op) for op in instruction.operands)}"
                if instruction.immediate is not None:
                    line += f", #{instruction.immediate}"
                output_lines.append(line)
        else:
            # Native disassembly
            instructions = disasm.disassemble_native(binary_data, args.format, args.base_address)
            output_lines = []
            
            for instr in instructions:
                line = f"{instr['mnemonic']}"
                if instr['op_str']:
                    line += f" {instr['op_str']}"
                output_lines.append(f"; 0x{instr['address']:08x}: {line}")
        
        # Output handling
        if args.output:
            with open(args.output, 'w') as f:
                f.write('; MIGHF-V3LC Disassembly\n')
                f.write(f'; Source: {args.input}\n')
                f.write(f'; Format: {args.format}\n\n')
                for line in output_lines:
                    f.write(line + '\n')
            print(f"Disassembly written to {args.output}")
        else:
            for line in output_lines:
                print(line)
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def simulate_main():
    """Main entry point for mighf-sim command"""
    parser = argparse.ArgumentParser(
        description='MIGHF-V3LC Simulator',
        prog='mighf-sim'
    )
    parser.add_argument('input', help='Input file (assembly or binary)')
    parser.add_argument('-t', '--type', choices=['asm', 'bin'], 
                       help='Input type (auto-detected if not specified)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    parser.add_argument('--max-cycles', type=int, default=1000000,
                       help='Maximum execution cycles')
    parser.add_argument('--debug', action='store_true',
                       help='Enable step-by-step debugging')
    parser.add_argument('--trace', help='Output execution trace to file')
    parser.add_argument('--memory-size', type=int, default=1024*1024,
                       help='Memory size in bytes')
    parser.add_argument('--dump-memory', help='Dump memory to file after execution')
    
    args = parser.parse_args()
    setup_logging(args.verbose)
    
    try:
        # Create simulator
        mighf = MIGHFV3LC(memory_size=args.memory_size)
        
        # Determine input type
        input_type = args.type
        if not input_type:
            input_type = 'asm' if args.input.endswith('.asm') else 'bin'
        
        # Load program
        if input_type == 'asm':
            with open(args.input, 'r') as f:
                assembly_code = f.read()
            instructions = mighf.load_assembly(assembly_code)
            print(f"Loaded {len(instructions)} instructions from assembly")
        else:
            with open(args.input, 'rb') as f:
                binary_data = f.read()
            mighf.load_binary(binary_data)
            print(f"Loaded {len(binary_data)} bytes of binary code")
        
        # Setup tracing if requested
        trace_file = None
        if args.trace:
            trace_file = open(args.trace, 'w')
            trace_file.write("# MIGHF-V3LC Execution Trace\n")
            trace_file.write("# Cycle PC Instruction Registers\n")
        
        # Execute program
        if args.debug:
            print("Debug mode - press Enter to step, 'q' to quit, 'r' to show registers")
            cycle = 0
            while mighf.cpu.running and cycle < args.max_cycles:
                state = mighf.get_state()
                pc = state['pc']
                
                # Fetch current instruction for display
                try:
                    instr_data = mighf.memory.read(pc, 4)
                    from mighf_v3lc import Instruction
                    current_instr = Instruction.decode(instr_data, pc)
                    print(f"Cycle {cycle}: PC=0x{pc:08x} | {current_instr.opcode.name}")
                except:
                    print(f"Cycle {cycle}: PC=0x{pc:08x} | <invalid>")
                
                # Debug prompt
                user_input = input("debug> ").strip().lower()
                if user_input == 'q':
                    break
                elif user_input == 'r':
                    print("Registers:")
                    for reg, val in state['registers'].items():
                        if val != 0:
                            print(f"  {reg}: {val} (0x{val:08x})")
                    continue
                
                # Step execution
                if not mighf.step_debug():
                    break
                
                # Write trace
                if trace_file:
                    state_after = mighf.get_state()
                    trace_file.write(f"{cycle} 0x{pc:08x} {current_instr.opcode.name} ")
                    trace_file.write(" ".join(f"{reg}={val}" for reg, val in state_after['registers'].items() if val != 0))
                    trace_file.write("\n")
                
                cycle += 1
        else:
            # Run normally
            cycles = mighf.execute_program(args.max_cycles)
            print(f"Execution completed in {cycles} cycles")
        
        # Show final state
        final_state = mighf.get_state()
        print("\nFinal CPU State:")
        print(f"  Cycles: {final_state['cycle_count']}")
        print(f"  PC: 0x{final_state['pc']:08x}")
        print(f"  Running: {final_state['running']}")
        print("  Registers:")
        for reg, val in final_state['registers'].items():
            if val != 0:
                print(f"    {reg}: {val} (0x{val:08x})")
        
        # Dump memory if requested
        if args.dump_memory:
            with open(args.dump_memory, 'wb') as f:
                f.write(mighf.memory.data)
            print(f"Memory dumped to {args.dump_memory}")
        
        # Close trace file
        if trace_file:
            trace_file.close()
            print(f"Execution trace written to {args.trace}")
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def analyze_main():
    """Main entry point for mighf-analyze command"""
    parser = argparse.ArgumentParser(
        description='MIGHF-V3LC Binary Analyzer',
        prog='mighf-analyze'
    )
    parser.add_argument('input', help='Input binary file')
    parser.add_argument('-o', '--output', help='Output analysis report')
    parser.add_argument('-f', '--format', choices=['json', 'text'], 
                       default='text', help='Output format')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    parser.add_argument('--symbolic', action='store_true',
                       help='Perform symbolic execution analysis')
    parser.add_argument('--cfg', action='store_true',
                       help='Generate control flow graph')
    
    args = parser.parse_args()
    setup_logging(args.verbose)
    
    try:
        # Create analyzers
        binary_parser = BinaryParser()
        
        # Read binary
        with open(args.input, 'rb') as f:
            binary_data = f.read()
        
        analysis_result = {
            'filename': args.input,
            'size': len(binary_data),
            'analysis_type': 'mighf-v3lc'
        }
        
        # Basic binary analysis
        try:
            if binary_data.startswith(b'\x7fELF'):
                elf_info = binary_parser.parse_elf(binary_data)
                analysis_result['format'] = 'ELF'
                analysis_result['elf_info'] = elf_info
            else:
                mighf_info = binary_parser.parse_mighf_binary(binary_data)
                analysis_result['format'] = 'MIGHF'
                analysis_result['mighf_info'] = mighf_info
        except Exception as e:
            analysis_result['parse_error'] = str(e)
        
        # Symbolic execution analysis
        if args.symbolic:
            try:
                symbolic_executor = SymbolicExecutor()
                if symbolic_executor.angr_available:
                    symbolic_analysis = symbolic_executor.analyze_binary(args.input)
                    analysis_result['symbolic_analysis'] = symbolic_analysis
                else:
                    analysis_result['symbolic_analysis'] = 'angr not available'
            except Exception as e:
                analysis_result['symbolic_error'] = str(e)
        
        # Output results
        if args.output:
            with open(args.output, 'w') as f:
                if args.format == 'json':
                    json.dump(analysis_result, f, indent=2, default=str)
                else:
                    f.write("MIGHF-V3LC Binary Analysis Report\n")
                    f.write("=" * 40 + "\n\n")
                    f.write(f"File: {analysis_result['filename']}\n")
                    f.write(f"Size: {analysis_result['size']} bytes\n")
                    f.write(f"Format: {analysis_result.get('format', 'Unknown')}\n\n")
                    
                    # Write detailed analysis
                    for key, value in analysis_result.items():
                        if key not in ['filename', 'size', 'format']:
                            f.write(f"{key}:\n{json.dumps(value, indent=2, default=str)}\n\n")
            
            print(f"Analysis report written to {args.output}")
        else:
            if args.format == 'json':
                print(json.dumps(analysis_result, indent=2, default=str))
            else:
                print("MIGHF-V3LC Binary Analysis Report")
                print("=" * 40)
                print(f"File: {analysis_result['filename']}")
                print(f"Size: {analysis_result['size']} bytes")
                print(f"Format: {analysis_result.get('format', 'Unknown')}")
                print()
                
                for key, value in analysis_result.items():
                    if key not in ['filename', 'size', 'format']:
                        print(f"{key}:")
                        if isinstance(value, dict):
                            for k, v in value.items():
                                print(f"  {k}: {v}")
                        else:
                            print(f"  {value}")
                        print()
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    # For direct execution, provide a simple interface
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python -m mighf_v3lc.cli <command>")
        print("Commands: assemble, disassemble, simulate, analyze")
        sys.exit(1)
    
    command = sys.argv[1]
    sys.argv = [f'mighf-{command}'] + sys.argv[2:]
    
    if command == 'assemble' or command == 'asm':
        assemble_main()
    elif command == 'disassemble' or command == 'disasm':
        disassemble_main()
    elif command == 'simulate' or command == 'sim':
        simulate_main()
    elif command == 'analyze':
        analyze_main()
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)
