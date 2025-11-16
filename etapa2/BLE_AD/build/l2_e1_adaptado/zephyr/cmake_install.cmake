# Install script for directory: C:/ncs/v3.1.0/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/ncs/toolchains/c1a76fddb2/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/arch/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/lib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/soc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/boards/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/subsys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/drivers/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/nrf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/hostap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/mcuboot/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/mbedtls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/trusted-firmware-m/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/cjson/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/azure-sdk-for-c/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/cirrus-logic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/openthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/memfault-firmware-sdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/canopennode/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/chre/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/lz4/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/nanopb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/zscilib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/cmsis/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/cmsis-dsp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/cmsis-nn/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/cmsis_6/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/fatfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/hal_nordic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/hal_st/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/hal_tdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/hal_wurthelektronik/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/liblc3/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/libmetal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/littlefs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/loramac-node/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/lvgl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/mipi-sys-t/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/nrf_wifi/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/open-amp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/percepio/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/picolibc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/segger/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/tinycrypt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/uoscore-uedhoc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/zcbor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/nrfxlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/nrf_hw_models/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/modules/connectedhomeip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/kernel/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/cmake/flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/cmake/usage/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/erics/OneDrive/Documentos/EMBARCATECH_FASE_3/Projetos/l2_e1_adaptado/build/l2_e1_adaptado/zephyr/cmake/reports/cmake_install.cmake")
endif()

