################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/device.c \
../src/interact.c \
../src/main.c \
../src/print.c \
../src/tcp_server.c 

OBJS += \
./src/device.o \
./src/interact.o \
./src/main.o \
./src/print.o \
./src/tcp_server.o 

C_DEPS += \
./src/device.d \
./src/interact.d \
./src/main.d \
./src/print.d \
./src/tcp_server.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/user/workspace/camera/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


