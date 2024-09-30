#[no_mangle]
pub extern "C" fn hello_from_rust() {
    let hex_string = hex::encode("Hello world!");
    println!("Hello from Rust! The hex string is {}.", hex_string);
}
