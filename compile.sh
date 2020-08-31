#!/bin/bash

# 编译token合约命令 compile.sh token
# 编译store合约命令 compile.sh store

if [ x"$1" = x ]; then 
  echo "请输入编译的合约名!"
  exit 1
fi

echo "------------------------------------------------------------------ 开始编译${1}.mta ------------------------------------------------------------------"

eosio-cpp -abigen -I store/include -I token/include -contract ${1} -o ${1}.wasm ./${1}/src/${1}.cpp
