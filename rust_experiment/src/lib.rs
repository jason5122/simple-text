extern crate aho_corasick;
extern crate hex;

use aho_corasick::{AhoCorasick, PatternID};
use std::fmt;
use std::ops::Add;

#[no_mangle]
pub extern "C" fn hello_from_rust() {
    let hex_string = hex::encode("Hello world!");
    println!("Hello from Rust! The hex string is {}.", hex_string);

    let p = Point { x: 3, y: 4 } + Point { x: 5, y: 6 };
    println!("{}", p);

    let patterns = &["apple", "maple", "snapple"];
    let haystack = "Nobody likes maple in their apple flavored Snapple.";

    let ac = AhoCorasick::builder()
        .ascii_case_insensitive(true)
        .build(patterns)
        .unwrap();
    let mut matches = vec![];
    for mat in ac.find_iter(haystack) {
        matches.push((mat.pattern(), mat.start(), mat.end()));
    }
    assert_eq!(
        matches,
        vec![
            (PatternID::must(1), 13, 18),
            (PatternID::must(0), 28, 33),
            (PatternID::must(2), 43, 50),
        ]
    );
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
