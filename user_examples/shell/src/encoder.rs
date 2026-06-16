use enum_dispatch::enum_dispatch;

#[enum_dispatch(TerminalEncoder)]
pub trait TextEncoder: Default {
    fn encode(&mut self, byte: u8) -> Option<char>;
}

#[enum_dispatch]
pub enum TerminalEncoder {
    Latin1(Latin1Encoder),
    Utf8(Utf8Encoder),
}
impl Default for TerminalEncoder {
    fn default() -> Self {
        Self::Latin1(Latin1Encoder::default())
    }
}

#[derive(Default)]
pub struct Latin1Encoder;
impl TextEncoder for Latin1Encoder {
    fn encode(&mut self, byte: u8) -> Option<char> {
        char::from_u32(byte as u32)
    }
}

#[derive(Default)]
pub struct Utf8Encoder {
    buf: [u8; 4],
    expected: usize,
    collected: usize,
}
impl Utf8Encoder {
    fn reset(&mut self) {
        *self = Self::default();
    }
}
impl TextEncoder for Utf8Encoder {
    fn encode(&mut self, byte: u8) -> Option<char> {
        if self.collected == 0 {
            self.expected = match byte {
                0x00..=0x7F => 1,
                0xC0..=0xDF => 2,
                0xE0..=0xEF => 3,
                0xF0..=0xF7 => 4,
                _ => {
                    self.reset();
                    return None;
                }
            }
        }

        self.buf[self.collected] = byte;
        self.collected += 1;

        if self.collected == self.expected {
            let ch = core::str::from_utf8(&self.buf[..self.expected])
                .ok()
                .and_then(|s| s.chars().next());

            self.reset();

            return ch;
        }

        None
    }
}
