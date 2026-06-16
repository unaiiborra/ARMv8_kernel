#![no_std]

use crate::{line_editor::LineEditor, parser::Args, terminal::input::TermInput};

extern crate stl;

pub mod dispatcher;
pub mod encoder;
pub mod line_editor;
pub mod parser;
pub mod terminal;

#[unsafe(no_mangle)]
extern "C" fn main() {
    let mut term_input = TermInput::default();
    let mut line_editor = LineEditor::default();

    loop {
        let Some(byte) = term_input.receive() else {
            continue;
        };

        let Some(line) = line_editor.edit(byte) else {
            continue;
        };

        let args = Args::parse_command_line(line);

        dispatcher::dispatch(args);
    }
}
