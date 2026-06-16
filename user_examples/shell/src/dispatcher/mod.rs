mod commands;

use crate::{
    parser::Args,
    terminal::output::{TermCommand::NewLine, TermOutput},
};

pub fn dispatch(args: Args) {
    let mut out = TermOutput::default();
    for a in args.iter() {
        out.add_str(a);
        out.push(NewLine(1));
    }

    out.send();
}
