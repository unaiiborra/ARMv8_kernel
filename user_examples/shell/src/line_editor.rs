use crate::{
    encoder::{TerminalEncoder, TextEncoder},
    terminal::output::{TermCommand, TermOutput},
};
use core::{mem, str::Chars};
use stl::alloc::{string::String, vec::Vec};

pub struct CommandLine(String);
impl CommandLine {
    pub fn chars(&'_ self) -> Chars<'_> {
        self.0.chars()
    }
}

#[derive(Default)]
pub struct LineEditor {
    encoder: TerminalEncoder,
    line: String,
    under_escape: bool,
    escape_data: String,
    history: Vec<String>,
    history_idx: Option<usize>,
}

impl LineEditor {
    pub fn set_encoder(&mut self, mode: TerminalEncoder) {
        self.encoder = mode;
    }

    pub fn edit(&mut self, byte: u8) -> Option<CommandLine> {
        let ch = self.encoder.encode(byte)?;

        if self.under_escape {
            self.handle_escape(ch);
            return None;
        }

        if !ch.is_control() {
            self.line.push(ch);
            TermOutput::send_char(ch);

            None
        } else {
            self.handle_control(ch)
        }
    }

    fn handle_control(&mut self, ch: char) -> Option<CommandLine> {
        match ch {
            // Enter
            '\n' | '\r' => {
                TermCommand::NewLine(1).send();

                if !self.line.is_empty()
                    && !(self.line.ends_with('\n') || self.line.ends_with('\r'))
                {
                    self.history.push(self.line.clone());
                    self.history_idx = None;

                    return Some(CommandLine(mem::take(&mut self.line)));
                }
            }
            // DEL
            '\x7F' | '\x08' => {
                if self.line.pop().is_some() {
                    TermCommand::Erase(1).send();
                }
            }
            // Interrupt (Ctrl+C)
            '\x03' => {
                self.line.clear();
                TermCommand::NewLine(1).send();
            }
            // EOF (Ctrl+D)
            '\x04' => {
                if self.line.is_empty() {
                    TermOutput::send_char('\x04');
                }
            }
            // Erase line (Ctrl+U )
            '\x15' => {
                TermCommand::Erase(self.line.chars().count()).send();
                self.line.clear();
            }
            // Erase last word (Ctrl+W)
            '\x17' => {
                while self.line.ends_with(' ') {
                    self.line.pop();
                    TermCommand::Erase(1).send();
                }

                while !self.line.is_empty() && !self.line.ends_with(' ') {
                    self.line.pop();
                    TermCommand::Erase(1).send();
                }
            }
            // Clean screen (Ctrl+L)
            '\x0C' => {
                TermCommand::ClearScreen.send();
                TermCommand::Str(self.line.as_str()).send();
            }
            // ESC
            '\x1B' => {
                self.under_escape = true;
            }
            _ => {}
        };

        None
    }

    fn handle_escape(&mut self, ch: char) {
        // TODO
        if ch.is_alphabetic() {
            self.under_escape = false;
        }
    }
}
