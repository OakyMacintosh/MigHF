meta:
  id: mighf_o_v2
  title: Mighf-O2 Binary Format
  file-extension: mbin
  endian: le
  ks-version: 0.9

seq:
  - id: header
    type: header
  - id: code
    type: instruction
    repeat: eos

types:
   header:
    seq:
      - id: magic
        contents: [0x4d, 0x49, 0x47, 0x48] # "MIGH"
      - id: version
	type: u2
      - id: entry_point
	type: u4
      - id: code_size
	type: u4

   instruction:
    seq:
      - id: raw
	type: u4
    instances:
      opcode:
         value: raw & 0x7f
      rd:
	 value: (raw >> 7) & 0x1F
      rs1:
	 value: (raw >> 15) & 0x1F
      rs2:
	 value: (raw >> 20) & 0x1F
      funct7:
	 value: (raw >> 25) & 0x7F
