#!/usr/bin/env python3
"""
Setup script for MIGHF-V3LC Micro-Architecture Library
"""

from setuptools import setup, find_packages
from setuptools.extension import Extension
import sys
import os

# Check for Cython
try:
    from Cython.Build import cythonize
    from Cython.Distutils import build_ext
    USE_CYTHON = True
except ImportError:
    USE_CYTHON = False

# Package metadata
NAME = "mighf-v3lc"
VERSION = "1.0.0"
DESCRIPTION = "Micro-Architecture for Instruction Generation and Hardware Functionality - Version 3 Low Complexity"
LONG_DESCRIPTION = """
MIGHF-V3LC is a comprehensive micro-architecture simulation library written in Python.
It provides a complete instruction set architecture with support for:

- Custom assembly language parsing and compilation
- Binary disassembly and analysis
- Symbolic execution capabilities
- Memory management and CPU simulation
- Multiple binary format parsing
- Performance optimization through Cython

The library integrates multiple powerful tools:
- angr for symbolic execution and binary analysis
- Capstone for disassembly
- Lark and pyparsing for language parsing
- KaitaiStruct for binary format parsing
- Cython for performance optimization

Features:
- Complete CPU simulation with registers, memory, and instruction execution
- Assembly language compiler and disassembler
- Symbolic execution engine integration
- Binary format parsing (ELF, custom formats)
- Debugging and profiling capabilities
- Extensible instruction set architecture
- Memory management with watchpoints and breakpoints
"""

AUTHOR = "MIGHF Development Team"
AUTHOR_EMAIL = "dev@mighf.org"
URL = "https://github.com/mighf/mighf-v3lc"
LICENSE = "MIT"

# Requirements
INSTALL_REQUIRES = [
    "lark>=1.1.0",
    "pyparsing>=3.0.0",
]

EXTRAS_REQUIRE = {
    'full': [
        "angr>=9.0.0",
        "capstone>=4.0.0",
        "kaitaistruct>=0.10",
        "cython>=0.29.0",
    ],
    'analysis': [
        "angr>=9.0.0",
    ],
    'disasm': [
        "capstone>=4.0.0",
    ],
    'parsing': [
        "kaitaistruct>=0.10",
    ],
    'performance': [
        "cython>=0.29.0",
    ],
    'dev': [
        "pytest>=6.0.0",
        "pytest-cov>=2.0.0",
        "black>=21.0.0",
        "flake8>=3.8.0",
        "mypy>=0.800",
    ]
}

# Cython extensions
ext_modules = []
if USE_CYTHON:
    # Define Cython extensions for performance-critical components
    extensions = [
        Extension(
            "mighf_v3lc.cpu_core",
            ["mighf_v3lc/cpu_core.pyx"],
            extra_compile_args=["-O3", "-ffast-math"],
        ),
        Extension(
            "mighf_v3lc.memory_core",
            ["mighf_v3lc/memory_core.pyx"],
            extra_compile_args=["-O3", "-ffast-math"],
        ),
        Extension(
            "mighf_v3lc.instruction_core",
            ["mighf_v3lc/instruction_core.pyx"],
            extra_compile_args=["-O3", "-ffast-math"],
        ),
    ]
    
    # Only add extensions if .pyx files exist (for development)
    existing_extensions = []
    for ext in extensions:
        pyx_file = ext.sources[0]
        if os.path.exists(pyx_file):
            existing_extensions.append(ext)
    
    if existing_extensions:
        ext_modules = cythonize(existing_extensions, compiler_directives={
            'language_level': 3,
            'boundscheck': False,
            'wraparound': False,
            'cdivision': True,
        })

# Custom build command for Cython
cmdclass = {}
if USE_CYTHON:
    cmdclass['build_ext'] = build_ext

# Classifiers
CLASSIFIERS = [
    "Development Status :: 4 - Beta",
    "Intended Audience :: Developers",
    "Intended Audience :: Education",
    "Intended Audience :: Science/Research",
    "License :: OSI Approved :: MIT License",
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.8",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
    "Programming Language :: Cython",
    "Topic :: Software Development :: Assemblers",
    "Topic :: Software Development :: Compilers",
    "Topic :: Software Development :: Debuggers",
    "Topic :: Software Development :: Disassemblers",
    "Topic :: System :: Emulators",
    "Topic :: Scientific/Engineering",
    "Operating System :: OS Independent",
]

# Entry points for command-line tools
ENTRY_POINTS = {
    'console_scripts': [
        'mighf-asm=mighf_v3lc.cli:assemble_main',
        'mighf-disasm=mighf_v3lc.cli:disassemble_main',
        'mighf-sim=mighf_v3lc.cli:simulate_main',
        'mighf-analyze=mighf_v3lc.cli:analyze_main',
    ],
}

def read_file(filename):
    """Read file contents"""
    with open(filename, 'r', encoding='utf-8') as f:
        return f.read()

# Read README if it exists
readme_content = LONG_DESCRIPTION
if os.path.exists('README.md'):
    readme_content = read_file('README.md')
elif os.path.exists('README.rst'):
    readme_content = read_file('README.rst')

setup(
    name=NAME,
    version=VERSION,
    description=DESCRIPTION,
    long_description=readme_content,
    long_description_content_type='text/markdown',
    author=AUTHOR,
    author_email=AUTHOR_EMAIL,
    url=URL,
    license=LICENSE,
    
    # Package configuration
    packages=find_packages(),
    py_modules=['mighf_v3lc'] if not find_packages() else [],
    include_package_data=True,
    zip_safe=False,
    
    # Dependencies
    python_requires='>=3.8',
    install_requires=INSTALL_REQUIRES,
    extras_require=EXTRAS_REQUIRE,
    
    # Cython extensions
    ext_modules=ext_modules,
    cmdclass=cmdclass,
    
    # Entry points
    entry_points=ENTRY_POINTS,
    
    # Metadata
    classifiers=CLASSIFIERS,
    keywords='microarchitecture simulation assembly disassembly cpu emulation',
#    project_urls={
#        'Bug Reports': 'https://github.com/mighf/mighf-v3lc/issues',
 #       'Source': 'https://github.com/mighf/mighf-v3lc',
  #      'Documentation': 'https://mighf-v3lc.readthedocs.io/',
   # },
    
    # Package data
    package_data={
        'mighf_v3lc': [
            '*.pyx',
            'data/*.json',
            'templates/*.txt',
            'examples/*.asm',
            'examples/*.py',
        ],
    },
)
