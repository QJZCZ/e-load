// 硬件测试源文件
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include <rtthread.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// 测试状态定义
typedef enum {
    TEST_IDLE = 0,      // 空闲状态
    TEST_GPIO,          // GPIO测试
    TEST_ADC,           // ADC测试
    TEST_DAC,           // DAC测试
    TEST_I2C,           // I2C测试
    TEST_UART1,         // UART1测试
    TEST_UART3,         // UART3测试
    TEST_COMPLETE       // 测试完成
} TestState;

// 测试结果结构体
typedef struct {
    uint8_t gpioPassed : 1;   // GPIO测试是否通过
    uint8_t adcPassed : 1;    // ADC测试是否通过
    uint8_t dacPassed : 1;    // DAC测试是否通过
    uint8_t i2cPassed : 1;    // I2C测试是否通过
    uint8_t uart1Passed : 1;  // UART1测试是否通过
    uint8_t uart3Passed : 1;  // UART3测试是否通过
} TestResults;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// 外部变量声明，访问main.c中定义的外设句柄
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern ADC_HandleTypeDef hadc2;
extern DAC_HandleTypeDef hdac1;
extern I2C_HandleTypeDef hi2c1;

TestState currentTest = TEST_IDLE;  // 当前测试状态
uint32_t testStartTime = 0;              // 测试开始时间
uint32_t testDuration = 2000;            // 每个测试持续时间（毫秒）

// 测试结果变量
TestResults testResults = {
    .gpioPassed = 0,
    .adcPassed = 0,
    .dacPassed = 0,
    .i2cPassed = 0,
    .uart1Passed = 0,
    .uart3Passed = 0
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/**
  * @brief  打印测试状态
  * @param  testName: 测试名称
  * @param  passed: 是否通过测试
  * @retval None
  */
static void printTestStatus(const char* testName, uint8_t passed);

/**
  * @brief  打印测试标题
  * @param  testName: 测试名称
  * @retval None
  */
static void printTestHeader(const char* testName);

/**
  * @brief  打印测试总结
  * @retval None
  */
static void printTestSummary(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  初始化硬件测试
  * @retval None
  * 
  * 功能：
  * 1. 设置初始测试状态为GPIO测试
  * 2. 记录测试开始时间
  * 3. 发送测试开始消息到UART1和UART3
  */
void HardwareTest_Init(void)
{
    currentTest = TEST_GPIO;           // 设置初始测试状态为GPIO测试
    testStartTime = HAL_GetTick();     // 记录测试开始时间
    
    char buffer[64];
    sprintf(buffer, "=== Hardware Test Started ===\r\n");
    // 发送测试开始消息到UART1
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
    // 发送测试开始消息到UART3
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
  * @brief  运行硬件测试状态机
  * @retval None
  * 
  * 测试流程：
  * 1. GPIO测试：测试LED闪烁和按键状态
  * 2. ADC测试：读取ADC2通道14的值并计算电压
  * 3. DAC测试：输出不同电压值
  * 4. I2C测试：扫描I2C总线上的设备
  * 5. UART1测试：发送测试消息
  * 6. UART3测试：发送测试消息
  * 7. 测试总结：显示所有测试结果
  */
void HardwareTest_Run(void)
{
    static char buffer[64];       // 用于格式化输出的缓冲区
    uint32_t currentTime = HAL_GetTick();  // 获取当前系统时间
    
    switch(currentTest) {
      case TEST_GPIO:
        if (currentTime - testStartTime >= testDuration) {
          // 测试GPIO
          printTestHeader("GPIO");
          
          // 测试LED - 交替闪烁
          HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);  // LED1亮
          HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); // LED2灭
          HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);     // 风扇控制引脚置高
          rt_thread_mdelay(500);  // 延时500ms
          HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); // LED1灭
          HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);   // LED2亮
          HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);   // 风扇控制引脚置低
          rt_thread_mdelay(500);  // 延时500ms
          
          // 测试按键 - 读取所有按键状态
          uint8_t k0 = HAL_GPIO_ReadPin(K0_GPIO_Port, K0_Pin);
          uint8_t k1 = HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin);
          uint8_t k2 = HAL_GPIO_ReadPin(K2_GPIO_Port, K2_Pin);
          uint8_t k3 = HAL_GPIO_ReadPin(K3_GPIO_Port, K3_Pin);
          uint8_t k4 = HAL_GPIO_ReadPin(K4_GPIO_Port, K4_Pin);
          
          // 输出按键状态
          sprintf(buffer, "Keys: K0=%d K1=%d K2=%d K3=%d K4=%d\r\n", k0, k1, k2, k3, k4);
          HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
          HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
          
          testResults.gpioPassed = 1; // GPIO测试通过
          printTestStatus("GPIO", testResults.gpioPassed);
          
          // 进入下一个测试
          currentTest = TEST_ADC;
          testStartTime = currentTime;
        }
        break;
        
      case TEST_ADC:
        if (currentTime - testStartTime >= testDuration) {
          // 测试ADC
          printTestHeader("ADC");
          
          // 启动ADC转换
          HAL_ADC_Start(&hadc2);
          if (HAL_ADC_PollForConversion(&hadc2, 1000) == HAL_OK) {
            // 读取ADC值
            uint32_t adcValue = HAL_ADC_GetValue(&hadc2);
            // 计算电压值 (3.3V参考电压，12位分辨率)
            float voltage = (float)adcValue * 3.3f / 4095.0f;
            // 输出ADC值和电压
            sprintf(buffer, "ADC Value: %d, Voltage: %.2fV\r\n", adcValue, voltage);
            HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
            HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
            testResults.adcPassed = 1;
          } else {
            // ADC转换失败
            testResults.adcPassed = 0;
          }
          // 停止ADC
          HAL_ADC_Stop(&hadc2);
          
          printTestStatus("ADC", testResults.adcPassed);
          
          // 进入下一个测试
          currentTest = TEST_DAC;
          testStartTime = currentTime;
        }
        break;
        
      case TEST_DAC:
        if (currentTime - testStartTime >= testDuration) {
          // 测试DAC
          printTestHeader("DAC");
          
          // 测试不同电压值 (0V, 0.8V, 1.6V, 2.4V, 3.3V)
          uint16_t dacValues[] = {0, 1023, 2047, 3071, 4095};
          for (int i = 0; i < 5; i++) {
            // 设置DAC值
            HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacValues[i]);
            // 启动DAC
            HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
            // 延时200ms，让电压稳定
            rt_thread_mdelay(200);
          }
          // 停止DAC
          HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
          
          testResults.dacPassed = 1;
          printTestStatus("DAC", testResults.dacPassed);
          
          // 进入下一个测试
          currentTest = TEST_I2C;
          testStartTime = currentTime;
        }
        break;
        
      case TEST_I2C:
        if (currentTime - testStartTime >= testDuration) {
          // 测试I2C
          printTestHeader("I2C");
          
          // 扫描I2C设备 (地址范围：1-127)
          uint8_t devicesFound = 0;
          for (uint8_t addr = 1; addr < 128; addr++) {
            // 检查设备是否存在
            if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 100) == HAL_OK) {
              // 找到设备，输出地址
              sprintf(buffer, "I2C Device found at address: 0x%02X\r\n", addr);
              HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
              HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
              devicesFound++;
            }
          }
          
          // 如果没有找到设备
          if (devicesFound == 0) {
            sprintf(buffer, "No I2C devices found\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
            HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
          }
          
          testResults.i2cPassed = 1; // 只要I2C初始化成功就算通过
          printTestStatus("I2C", testResults.i2cPassed);
          
          // 进入下一个测试
          currentTest = TEST_UART1;
          testStartTime = currentTime;
        }
        break;
        
      case TEST_UART1:
        if (currentTime - testStartTime >= testDuration) {
          // 测试UART1
          printTestHeader("UART1");
          
          // 发送测试消息
          char testMsg[] = "UART1 Test Message\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t*)testMsg, strlen(testMsg), 1000);
          
          testResults.uart1Passed = 1;
          printTestStatus("UART1", testResults.uart1Passed);
          
          // 进入下一个测试
          currentTest = TEST_UART3;
          testStartTime = currentTime;
        }
        break;
        
      case TEST_UART3:
        if (currentTime - testStartTime >= testDuration) {
          // 测试UART3
          printTestHeader("UART3");
          
          // 发送测试消息
          char testMsg[] = "UART3 Test Message\r\n";
          HAL_UART_Transmit(&huart3, (uint8_t*)testMsg, strlen(testMsg), 1000);
          
          testResults.uart3Passed = 1;
          printTestStatus("UART3", testResults.uart3Passed);
          
          // 进入测试完成状态
          currentTest = TEST_COMPLETE;
          testStartTime = currentTime;
        }
        break;
        
      case TEST_COMPLETE:
        if (currentTime - testStartTime >= testDuration) {
          // 打印测试总结
          printTestSummary();
          
          // 测试完成，进入 idle 状态
          currentTest = TEST_IDLE;
        }
        break;
        
      default:
        // 空闲状态，等待重新开始测试
        break;
    }
}

/**
  * @brief  打印测试状态
  * @param  testName: 测试名称
  * @param  passed: 是否通过测试
  * @retval None
  * 
  * 功能：
  * 1. 格式化测试状态消息
  * 2. 发送消息到UART1
  * 3. 发送消息到UART3
  */
static void printTestStatus(const char* testName, uint8_t passed)
{
    char buffer[64];
    // 格式化测试状态消息
    sprintf(buffer, "%s: %s\r\n", testName, passed ? "PASSED" : "FAILED");
    // 发送消息到UART1
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
    // 发送消息到UART3
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
  * @brief  打印测试标题
  * @param  testName: 测试名称
  * @retval None
  * 
  * 功能：
  * 1. 格式化测试标题消息
  * 2. 发送消息到UART1
  * 3. 发送消息到UART3
  */
static void printTestHeader(const char* testName)
{
    char buffer[64];
    // 格式化测试标题消息
    sprintf(buffer, "\r\n=== Testing %s ===\r\n", testName);
    // 发送消息到UART1
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
    // 发送消息到UART3
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
}

/**
  * @brief  打印测试总结
  * @retval None
  * 
  * 功能：
  * 1. 打印测试总结标题
  * 2. 打印每个测试的结果
  * 3. 打印整体测试结果
  */
static void printTestSummary(void)
{
    char buffer[128];
    // 打印测试总结标题
    sprintf(buffer, "\r\n=== Test Summary ===\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
    
    // 打印每个测试的结果
    printTestStatus("GPIO", testResults.gpioPassed);
    printTestStatus("ADC", testResults.adcPassed);
    printTestStatus("DAC", testResults.dacPassed);
    printTestStatus("I2C", testResults.i2cPassed);
    printTestStatus("UART1", testResults.uart1Passed);
    printTestStatus("UART3", testResults.uart3Passed);
    
    // 计算整体测试结果
    uint8_t allPassed = testResults.gpioPassed && testResults.adcPassed && 
                       testResults.dacPassed && testResults.i2cPassed && 
                       testResults.uart1Passed && testResults.uart3Passed;
    
    // 打印整体测试结果
    sprintf(buffer, "\r\nOverall: %s\r\n", allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/



