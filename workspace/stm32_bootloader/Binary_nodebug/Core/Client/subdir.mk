################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Client/bl_ota_helper.c 

OBJS += \
./Core/Client/bl_ota_helper.o 

C_DEPS += \
./Core/Client/bl_ota_helper.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Client/%.o Core/Client/%.su Core/Client/%.cyclo: ../Core/Client/%.c Core/Client/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Client

clean-Core-2f-Client:
	-$(RM) ./Core/Client/bl_ota_helper.cyclo ./Core/Client/bl_ota_helper.d ./Core/Client/bl_ota_helper.o ./Core/Client/bl_ota_helper.su

.PHONY: clean-Core-2f-Client

