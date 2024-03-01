use std::{
    io::{Read, Write},
    path::PathBuf,
};

fn main() {
    for arg in std::env::args()
        .skip(1)
        .filter(|p| !p.ends_with("_deblanked.bin") && p.ends_with(".bin"))
    {
        let path: PathBuf = arg.into();

        let new_stem = path.file_stem().unwrap().to_str().unwrap().to_owned() + "_deblanked.bin";
        let new_path =
            path.parent().unwrap().to_str().unwrap().to_owned() + "/" + new_stem.as_str();

        println!("{:?} {:?}", path, new_path);

        let data = {
            let mut data = Vec::new();
            let mut file = std::fs::File::open(path).unwrap();
            file.read_to_end(&mut data).unwrap();
            data
        };

        let mut writing = std::fs::File::create(new_path).unwrap();

        for second in 0..60 {
            let s = second * 4 * 3 * 500;
            let e = second * 4 * 3 * 500 + 60 * 4 * 3;
            writing.write_all(&data[s..e]).unwrap();
        }
    }
}
