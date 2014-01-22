################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MQTTDeserializeConnect.c \
../src/MQTTPacket.c \
../src/MQTTSerializeConnect.c 

C_DEPS += \
./src/MQTTDeserializeConnect.d \
./src/MQTTPacket.d \
./src/MQTTSerializeConnect.d 

OBJS += \
./src/MQTTDeserializeConnect.o \
./src/MQTTPacket.o \
./src/MQTTSerializeConnect.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


