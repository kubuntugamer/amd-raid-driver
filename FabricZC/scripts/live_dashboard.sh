#!/bin/bash
# FabricZC Core Lab: Live Hardware & Storage Telemetry Dashboard Execution Layer

while true; do
  clear
  echo -e "\e[1;36m=========================================================================\e[0m"
  echo -e "\e[1;36m   FABRICZC CORE ENGINE LAB: LIVE HARDWARE & STORAGE TELEMETRY GRID      \e[0m"
  echo -e "\e[1;36m=========================================================================\e[0m"
  
  # 1. PROCESSOR METRICS (RAPL Motherboard Registers)
  echo -e "\e[1;33m AMD RYZEN 7 9700X SILICON ENGINE\e[0m"
  e1=$(cat /sys/class/powercap/intel-rapl:0/energy_uj 2>/dev/null)
  t1=$(date +%s.%N)
  sleep 0.2
  e2=$(cat /sys/class/powercap/intel-rapl:0/energy_uj 2>/dev/null)
  t2=$(date +%s.%N)
  
  if [ ! -z "$e1" ] && [ ! -z "$e2" ]; then
    cpu_w=$(echo "scale=2; ($e2 - $e1) / ($t2 - $t1) / 1000000" | bc)
    echo "  ▸ Motherboard Pkg    : $cpu_w Watts"
  else
    echo "  ▸ Motherboard Pkg    : [Register Access Restricted / Sudo Required]"
  fi
  
  cpu_freq=$(lscpu | grep "CPU MHz" | awk '{print $3}')
  cpu_temp=$(cat /sys/class/thermal/thermal_zone*/temp 2>/dev/null | head -n 1 | awk '{print $1/1000}')
  echo "  ▸ Core Freq Baseline : ${cpu_freq:-0} MHz"
  echo "  ▸ Silicon Core Temp  : ${cpu_temp:-0} °C"
  
  # 2. GRAPHICS METRICS (NVIDIA Telemetry Interface)
  echo -e "\n\e[1;35m ZOTAC RTX 5050 HARDWARE RAIL\e[0m"
  gpu_raw=$(nvidia-smi --query-gpu=utilization.gpu,temperature.gpu,power.draw,clocks.current.graphics,fan.speed --format=csv,noheader,nounits 2>/dev/null)
  if [ ! -z "$gpu_raw" ]; then
    echo "  ▸ GPU Core Load      : $(echo $gpu_raw | awk -F', ' '{print $1}') %"
    echo "  ▸ Core Temperature   : $(echo $gpu_raw | awk -F', ' '{print $2}') °C"
    echo "  ▸ Electrical Draw    : $(echo $gpu_raw | awk -F', ' '{print $3}') Watts"
    echo "  ▸ Active Boost Clock : $(echo $gpu_raw | awk -F', ' '{print $4}') MHz"
    echo "  ▸ Locked Fan Speed   : $(echo $gpu_raw | awk -F', ' '{print $5}') %"
  else
    echo "  ▸ [NVIDIA Driver Telemetry Offline / Suspended]"
  fi

  # 3. KERNEL MEMORY CAPACITY (VFS Subsystem Tax)
  echo -e "\n\e[1;32m LINUX SYSTEM MEMORY & SLAB FOOTPRINT\e[0m"
  slab_unrec=$(cat /proc/meminfo | grep "SUnreclaim" | awk '{print $2/1024}')
  free_ram=$(free -m | grep "Mem:" | awk '{print $4}')
  echo "  ▸ Unreclaimable Slab : $(printf "%.2f" $slab_unrec) MB (VFS/Blob Structure Bloat)"
  echo "  ▸ Free Available RAM : $free_ram MB"

  # 4. STORAGE DATA BUS LAYER (desktopECHO rcraid / dev/rcraid0 Array)
  echo -e "\n\e[1;34m 16TB NVME STORAGE BUS ARRAY METRICS\e[0m"
  if [ -b /dev/rcraid0 ]; then
    io_status=$(cat /sys/block/rcraid0/stat)
    read_sectors=$(echo $io_status | awk '{print $3}')
    write_sectors=$(echo $io_status | awk '{print $7}')
    read_mb=$(echo "scale=2; $read_sectors * 512 / 1024 / 1024" | bc)
    write_mb=$(echo "scale=2; $write_sectors * 512 / 1024 / 1024" | bc)
    echo "  ▸ Active Target Node : /dev/rcraid0"
    echo "  ▸ Total Block Reads  : $read_mb MB"
    echo "  ▸ Total Block Writes : $write_mb MB"
  elif [ -b /dev/sda ]; then
    echo "  ▸ Active Target Node : /dev/sda (AMD Blob SCSI Emulation Emitted)"
  else
    echo "  ▸ [No Hardware Array Detected: Standard /dev/nvme Paths Trapped]"
  fi
  echo -e "\e[1;36m=========================================================================\e[0m"
  sleep 0.8
done
