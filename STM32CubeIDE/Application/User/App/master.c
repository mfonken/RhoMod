/************************************************************************
 *  File: master.c
 *  Group: Rho Core
 ***********************************************************************/

/***************************************************************************************/
/*                                    Includes                                         */
/***************************************************************************************/
#include "master.h"
#include "printers.h"

/***************************************************************************************/
/*                                  Core Routines                                      */
/***************************************************************************************/
system_states_list_t global_states_list =
{
	{ INITIALIZING,       CONNECTING_TO_HOST, InitializePlatform    },
	{ CONNECTING_TO_HOST, CONFIGURING,        ConnectToHost         },
	{ CONFIGURING,        READY,              ConfigureApplication  },
	{ READY,              ACTIVE,             ExitInitialization    },
	{ ACTIVE,             IDLE,               ApplicationCore       },
	{ SYS_ERROR,          IDLE,               SystemError           }
};

dma_info_t CameraDMA =
{
	RHO_TIM_IT_CC,
	RHO_TIM_CHANNEL,
	RHO_TIM_DMA_CC,
	(uint32_t)&CAMERA_PORT,
	(uint32_t)NULL,
	(uint16_t)CAPTURE_BUFFER_SIZE,
	RHO_TIM_DMA_ID,
	false
};
camera_application_flags CameraFlags = { 0 };
GPIO_t LED_GPIO = { LED_GPIO_Port, LED_Pin };
bool frame_ready;
/**
  * @brief This function handles DMA1 channel1 global interrupt.
  */
//void DMA1_Channel1_IRQHandler(void)
//{
//  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */
//
//  /* USER CODE END DMA1_Channel1_IRQn 0 */
//  HAL_DMA_IRQHandler(Master.Utilities.Timer_Primary->hdma[RHO_TIM_DMA_ID]);
//  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */
////  PlatformFunctions.GPIO.Toggle( &LED_GPIO );
//
//  frame_ready = true;
//  /* USER CODE END DMA1_Channel1_IRQn 1 */
//}

/* INITIALIZING State Routine */
void InitializePlatform( void )
{
	LOG(DEBUG_2, "Initializing platform"ENDL);
    PlatformFunctions.Init( &Platform, HOST_COMMUNICATION_PROTOCOL, (generic_handle_t)Master.IOs.HOST_DEFAULT_CHANNEL );
}

/* CONNECTING_TO_HOST State Routine */
void ConnectToHost( void )
{
#ifdef PERFORM_HOST_PING_FOR_INIT
  while( PlatformFunctions.Host.Command( PING_HOST, NEEDED ) != OK )
  {
    PlatformFunctions.Wait( HOST_COMMAND_WAIT_TIME );
  }
#endif
}

/* CONFIGURING State Routine */
void ConfigureApplication( void )
{
#ifdef __OV9712__
  OV9712_Functions.Init( &OV9712, Master.IOs.CAMERA_COMMUNICATION_CHANNEL, &Default_OV9712_Pins );
#endif
#ifdef __RHO__
  RhoSystem.Functions.Perform.ConnectToInterface( &PlatformFunctions, &CameraFlags, &CameraDMA );
  RhoSystem.Functions.Perform.Initialize( CAMERA_PORT, UART_TX_PORT );
#endif
}

/* READY State Routine */
void ExitInitialization( void )
{
#ifdef __RHO__
  RhoSystem.Functions.Perform.Activate();
#endif
  if( SystemFunctions.State.IsIn( &System, READY ) )
    MasterFunctions.Run();
  else
    PlatformFunctions.Wait(250);
}

/* ACTIVE State Routine */
void ApplicationCore( void )
{
#ifdef __RHO__
    RhoSystem.Functions.Perform.CoreProcess();
#else
#warning "No application core."
#endif
}

/* SYS_ERROR State Routine */
void SystemError( void )
{
    LOG( ALWAYS, "System error! Resetting in" );
    for( uint8_t i = 3 ; i > 0; i-- )
    {
        LOG_BARE( ALWAYS, " %d", i );
        PlatformFunctions.Wait(1000);
    }
    PlatformFunctions.Reset();
}

/***************************************************************************************/
/*                                Master Initialize                                    */
/***************************************************************************************/
void Master_Connect( I2C_Handle_t * i2c, TIMER_Handle_t * timer, UART_Handle_t * usart )
{
  printf("Connecting master..."ENDL);
  Master.IOs.I2C_Primary = i2c;
  Master.IOs.UART_Primary = usart;
  Master.Utilities.Timer_Primary = timer;
  PlatformFunctions.GPIO.Write( &LED_GPIO, GPIO_PIN_SET);
  MasterFunctions.Init();
}

void Master_Init( void )
{
  /* Initialize state manager */
  SystemFunctions.Init( &System, &global_states_list );

  /* Initialize core platform */
  SystemFunctions.State.Enter( &System, INITIALIZING );

  /* Connect to host, this is critical for finishing initialization hence endless loop */
  SystemFunctions.State.Enter( &System, CONNECTING_TO_HOST );

  /* Configure application items */
  SystemFunctions.State.Enter( &System, CONFIGURING );

  /* Exit initialization, enter run */
  SystemFunctions.State.Enter( &System, READY );
}

/***************************************************************************************/
/*                                   Master Run                                        */
/***************************************************************************************/
void Master_Run( void )
{
	SystemFunctions.State.Next( &System );
//	SystemFunctions.State.Set( &System, ACTIVE );

	while(1)
	{
		SystemFunctions.State.Perform( &System );
	}
}
