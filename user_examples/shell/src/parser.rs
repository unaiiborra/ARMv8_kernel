use core::mem;

use stl::alloc::{string::String, vec::Vec};

use crate::line_editor::CommandLine;

pub struct ArgsIter<'a> {
    args: &'a [String],
    idx: usize,
}
impl<'a> Iterator for ArgsIter<'a> {
    type Item = &'a str;

    fn next(&mut self) -> Option<Self::Item> {
        let s = self.args.get(self.idx)?;
        self.idx += 1;
        Some(s.as_str())
    }
}

pub struct Args(Vec<String>);
impl Args {
    pub fn parse_command_line(line: CommandLine) -> Self {
        let mut vec = Vec::<String>::new();
        let mut current = String::new();

        for c in line.chars() {
            if c == ' ' {
                if !current.is_empty() {
                    vec.push(mem::take(&mut current));
                }
            } else {
                current.push(c);
            }
        }

        if !current.is_empty() {
            vec.push(current);
        }

        Self(vec)
    }

    pub const fn argc(&self) -> usize {
        self.0.len()
    }

    pub fn argv(&self, idx: usize) -> Option<&String> {
        self.0.get(idx)
    }

    pub fn iter(&'_ self) -> ArgsIter<'_> {
        ArgsIter {
            args: &self.0,
            idx: 0,
        }
    }
}
