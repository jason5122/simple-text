use std::fmt;
use std::ops::Add;

#[no_mangle]
pub extern "C" fn hello_from_rust() {
    let hex_string = hex::encode("Hello world!");
    println!("Hello from Rust! The hex string is {}.", hex_string);

    let p = Point { x: 3, y: 4 } + Point { x: 5, y: 6 };
    println!("{}", p);
}

#[derive(Debug, Copy, Clone, PartialEq)] // PartialEq is probably the most important part here. You want to be able to compare numbers
struct Point {
    x: i32,
    y: i32,
}

impl Add for Point {
    type Output = Self; // Remember, this is called an "associated type": a "type that goes together".
                        // In this case it's just another Point

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
