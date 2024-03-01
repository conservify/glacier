use std::path::{Path, PathBuf};

use anyhow::Result;
use chrono::{DateTime, Utc};
use walkdir::WalkDir;

#[derive(Debug)]
struct BinarySamples {
    path: PathBuf,
    time: DateTime<Utc>,
}

impl BinarySamples {
    fn from_path<P: AsRef<Path>>(path: P) -> Option<BinarySamples> {
        let path_str = path.as_ref().to_string_lossy().to_owned();
        let parts = path_str.split("_").skip(1);

        println!("{:?}", parts);

        None
    }
}

fn main() -> Result<()> {
    for entry in WalkDir::new("/home/jlewallen/drive2/glacier/geophone")
        .into_iter()
        .filter_map(|e| e.ok())
        .filter(|e| e.path().extension().map(|e| e == "bin").unwrap_or_default())
        .filter_map(|e| BinarySamples::from_path(e.path()))
    {
        println!("{:?}", entry);
    }

    Ok(())
}
