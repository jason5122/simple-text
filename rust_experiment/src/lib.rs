extern crate proc_macro2;

use proc_macro2::{TokenStream, TokenTree};

#[no_mangle]
pub extern "C" fn hello_from_rust() {}
