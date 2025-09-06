use std::io;
use std::fs::{Write, Read};
use clap::Parser;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]

struct Args {
    //
}

pub fn RuntimeRS() -> bool {
    struct Runtime {
        name: String,
        version: String,
        is32: Num,
        author: String,
        repo: String,
    }

    
}

fn main() {
    //
}