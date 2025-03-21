ydb_dir="$(cd "$(dirname "${(%):-%N}")" && pwd)"
bin_path="${ydb_dir}/bin"
export PATH="${bin_path}:${PATH}"
