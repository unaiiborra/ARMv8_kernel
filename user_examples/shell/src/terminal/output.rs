use core::iter::empty;

use stl::{
    alloc::{string::String, vec::Vec},
    stdio::STDOUT_FD,
};

#[derive(Default)]
pub struct TermOutput<'a>(Vec<TermCommand<'a>>);
impl<'a> TermOutput<'a> {
    pub fn push(&mut self, command: TermCommand<'a>) {
        self.0.push(command);
    }

    fn write_bytes(bytes: &[u8]) {
        let mut remaining = bytes;

        while !remaining.is_empty() {
            let written = STDOUT_FD.write(remaining).expect("write failed");

            remaining = &remaining[written..];
        }
    }

    pub fn send(&mut self) -> &mut TermOutput<'a> {
        let mut text = String::new();

        for command in &self.0 {
            let (string, repeat) = command.as_str();

            for _ in 0..repeat {
                text.push_str(string);
            }
        }

        Self::write_bytes(text.as_bytes());
        self
    }

    pub fn add_str(&mut self, str: &'a str) -> &mut TermOutput<'a> {
        self.push(TermCommand::Str(str));
        self
    }

    pub fn flush(&mut self) {
        self.0.clear();
    }

    pub fn send_char(ch: char) {
        let mut buf = [0u8; 4];
        let s: &str = ch.encode_utf8(&mut buf);
        Self::write_bytes(s.as_bytes());
    }

    pub fn send_str(str: &str) {
        Self::write_bytes(str.as_bytes());
    }
}

#[derive(Clone)]
pub enum TermCommand<'a> {
    Str(&'a str),
    NewLine(usize),
    Erase(usize),
    MoveLeft(usize),
    MoveRight(usize),
    MoveUp(usize),
    MoveDown(usize),
    ClearToEndOfLine,
    ClearScreen,
    HideCursor,
    ShowCursor,
}
impl<'a> TermCommand<'a> {
    fn as_str(&self) -> (&'a str, usize) {
        match *self {
            TermCommand::Str(s) => (s, 1),
            TermCommand::ClearToEndOfLine => ("\x1B[K", 1),
            TermCommand::ClearScreen => ("\x1B[2J\x1B[H", 1),
            TermCommand::HideCursor => ("\x1B[?25l", 1),
            TermCommand::ShowCursor => ("\x1B[?25h", 1),
            TermCommand::NewLine(n) => ("\n\r", n),
            TermCommand::Erase(n) => ("\x08 \x08", n),
            TermCommand::MoveLeft(n) => ("\x1B[D", n),
            TermCommand::MoveRight(n) => ("\x1B[C", n),
            TermCommand::MoveUp(n) => ("\x1B[A", n),
            TermCommand::MoveDown(n) => ("\x1B[B", n),
        }
    }

    pub fn send(&self) {
        let (string, repeat) = self.as_str();
        let repeated = string.repeat(repeat);
        TermOutput::write_bytes(repeated.as_bytes());
    }
}
