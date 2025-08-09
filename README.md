<center><img src="logo.jpg" /></center>
    <center>The OpenSource ISA that doesn't differ.</center>
<hr>

*mighf* is a general purpose micro computer universal, and opensource micro-architecture made to be replicated and reimplemented for any platforms.

## Build instructions
Clone the repo
```bash
git clone https://oakymacintosh/mighf.git
cd mighf
```
Now go to the ISA2.0 dir. and build the softcpu
```bash
cd ISA2.0
gcc mhfsoft -o mhf
```
Done! Now you can run any programs compiled for *mighf* using the *mhf* softcpu.

## Developer Tools
There are not much devtools, since *mighf* is just a "baby" architecture. But you can code in *mighf* assembly, using the `mighf-unknown-gnuabi-as` assembler.

It can be builded by simply compiling the `toolchain/mighf-unknown-gnuabi-as.c` file with gcc.

take a look at the [wiki](https://github.com/OakyMacintosh/MigHF/wiki/Assembly-Language-Documentation-(Home)) for reference

## Contributing
I, OakyMacintosh acept any type of help in this project! Any questions, or help, or even bugs, can be reported and asked without going out of github.

> [!NOTE]
> I need someone who can contribute by creating pre-built Darwin (macOS) Binaries, since I don't own a Mac!
> contact [me](mailto:migsufrem@hotmail.com) for more info, and also by volluntaring yourself!
