use std::fmt;
use std::ops::Add;

#[no_mangle]
pub extern "C" fn hello_from_rust() {
    let hex_string = hex::encode("Hello world!");
    println!("Hello from Rust! The hex string is {}.", hex_string);

    let p = Point { x: 3, y: 4 } + Point { x: 5, y: 6 };
    println!("{}", p);
}

#[derive(Debug, Copy, Clone, PartialEq)]
struct Point {
    x: i32,
    y: i32,
}

impl Add for Point {
    type Output = Self;

    fn add(self, other: Self) -> Self {
        Self {
            x: self.x + other.x,
            y: self.y + other.y,
        }
    }
}

impl fmt::Display for Point {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Point({}, {})", self.x, self.y)
    }
}
