#!/bin/sh

# Configuração da rede
cp $BASE_DIR/../custom-scripts/S41network-config \
   $BASE_DIR/target/etc/init.d/S41network-config

chmod +x $BASE_DIR/target/etc/init.d/S41network-config


# Aplicação SystemInfo
cp $BASE_DIR/../custom-scripts/systeminfo.py \
   $BASE_DIR/target/usr/bin/systeminfo.py

chmod +x $BASE_DIR/target/usr/bin/systeminfo.py


# Inicialização automática do SystemInfo
cp $BASE_DIR/../custom-scripts/S50systeminfo \
   $BASE_DIR/target/etc/init.d/S50systeminfo

chmod +x $BASE_DIR/target/etc/init.d/S50systeminfo