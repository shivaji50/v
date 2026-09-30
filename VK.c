// Win32 headers
// link.exe WindowRepitrion_01.obj user32.lib gdi32.lib
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "VK.h"

// vulkan related header file
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>


// vulkan related library
#pragma comment(lib,"vulkan-1.lib")

// Macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// global function declarartions
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

const char* gpszAppName = "ARTR";

// global variable declarations
// variables related with fullscreen
BOOL gbFullScreen = FALSE;
HWND ghwnd = NULL;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev;

// variables related with File I/O
char gszLogFileName[] = "Log.txt";
FILE *gpFile = NULL;

// Active window related variable
BOOL gbActiveWindow = FALSE;

// Exit key press related
BOOL gbEscapeKeyIsPressed = FALSE;

// Vulkan related global variables

// Instance extension related variables
uint32_t enabledInstanceExtensionCount = 0;
// VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME
const char* enabledInstanceExtensionNames_array[2];
 
// Vulkan instance
VkInstance vkInstance = VK_NULL_HANDLE;

// vulkan presentation surface object
VkSurfaceKHR vkSurfaceKHR = VK_NULL_HANDLE;

// vulkan physical device related gloabal variable
VkPhysicalDevice vkPhysicalDevice_Selected = VK_NULL_HANDLE;
uint32_t graphicsQueueFamilyIndex_Selected = UINT32_MAX;
VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;


// Entry Point Function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	// local function declaration
	VkResult initialize(void);
	void display(void);
	void update(void);
	void uninitialize(void);


	// Variable declaration
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szAppName[256];
	BOOL bDone = FALSE;
	VkResult vkresult = VK_SUCCESS;


	// Code
	
	// Create log file
	gpFile = fopen(gszLogFileName, "w");
	if (gpFile == NULL)
	{
		MessageBox(NULL, TEXT("Log file creation failed"), TEXT("File I/O error"), MB_OK);
		exit(0);
	}
	else
	{
		fprintf(gpFile, "WinMain() -> Program Started Succesfully\n");
	}


	wsprintf(szAppName, TEXT("%s"), gpszAppName);


	// Window class Intialization
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = NULL;
	wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));


	// Registration of window class
	RegisterClassEx(&wndclass);

	// create window
	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		szAppName,
		TEXT("SD : VULKUN"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ghwnd = hwnd;

	//initialize
	vkresult = initialize();
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "WinMain() -> initialize() failed\n");
		DestroyWindow(hwnd);
		hwnd = NULL;
	}
	else
	{
		fprintf(gpFile, "WinMain() -> initialize() completed succesfully\n");
	}

	// show window
	ShowWindow(hwnd, iCmdShow);

	// paint backgroud of the window
	UpdateWindow(hwnd);



	// set this window as foreground and active window
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	// game loop
	while (bDone == FALSE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				bDone = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			if (gbActiveWindow == TRUE)
			{
				if (gbEscapeKeyIsPressed == TRUE)
				{
					bDone = TRUE;
				}

				// Render
				display();

				// Update
				update();
			}
			
		}
	}

	// unintialize
	uninitialize();
	
	return((int)msg.wParam);

}

// Call Back Function
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// local function declaration
	void toggleFullScreen(void);
	void resize(int, int);
	void uninitialize(void);


	// code
	switch (iMsg)
	{
	case WM_CREATE:
		ZeroMemory((void*)&wpPrev, sizeof(WINDOWPLACEMENT));
		wpPrev.length = sizeof(WINDOWPLACEMENT);
		break;
	case WM_SETFOCUS:
		gbActiveWindow = TRUE;
		break;
	case WM_KILLFOCUS:
		gbActiveWindow = FALSE;
		break;
	case WM_SIZE:
		resize(LOWORD(lParam),HIWORD(lParam));
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			gbEscapeKeyIsPressed = TRUE;
			break;
		default:
			break;
		}
		break;
	case WM_CHAR:
		switch (wParam)
		{
		case 'F':
		case 'f':
			if (gbFullScreen == FALSE)
			{
				toggleFullScreen();
				gbFullScreen = TRUE;
			}
			else
			{
				toggleFullScreen();
				gbFullScreen = FALSE;
			}
			break;
		default:
			break;
		}
		break;
	case WM_CLOSE:
		uninitialize();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return(DefWindowProc(hwnd, iMsg, wParam, lParam));

}

void toggleFullScreen(void)
{
	// variable declarations
	MONITORINFO mi;

	// code
	if (gbFullScreen == FALSE)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			ZeroMemory((void*)&mi, sizeof(MONITORINFO));
			mi.cbSize = sizeof(MONITORINFO);
			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);

			}
		}
		ShowCursor(FALSE);
	}
	else
	{
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
}

VkResult initialize(void)
{
	
	// function declarations
	VkResult createVulkanInstance(void);
	VkResult getSupportedSurface(void);
	VkResult getPhysicalDevice(void);

	// Variable declarations
	VkResult vkresult = VK_SUCCESS;

	// code

	// create vulkan instance
	vkresult = createVulkanInstance();
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "initialize() -> createVulkanInstance() failed %d\n",vkresult);
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "initialize() -> createVulkanInstance() completed succesfully\n");
	}

	// create vulkan presentation surface
	vkresult = getSupportedSurface();
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "initialize() -> getSupportedSurface() failed %d\n",vkresult);
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "initialize() -> getSupportedSurface() completed succesfully\n");
	}

	// select required physical device and its queue family index
	vkresult = getPhysicalDevice();
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "initialize() -> getPhysicalDevice() failed %d\n",vkresult);
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "initialize() -> getPhysicalDevice() completed succesfully\n");
	}

	return(vkresult);
}

void resize(int width, int height)
{
	// code

}

void display(void)
{
	// code

}

void update(void)
{
	// code

}

void uninitialize(void)
{
	//  code

	// 
	if(vkSurfaceKHR)
	{
		vkDestroySurfaceKHR(vkInstance,vkSurfaceKHR,NULL);
		vkSurfaceKHR = VK_NULL_HANDLE;
		fprintf(gpFile, "uninitialize() -> vkDestroySurfaceKHR() Succeded\n");
	}

	// Destroy vulkan instance
	if(vkInstance)
	{
		vkDestroyInstance(vkInstance,NULL);
		vkInstance = VK_NULL_HANDLE;
		fprintf(gpFile, "uninitialize() -> vkDestroyInstance() Succeded\n");
	}

	// close the file
	if (gpFile)
	{
		fprintf(gpFile, "uninitialize() -> Program Terminated sucessfully\n");
		fclose(gpFile);
		gpFile = NULL;
	}
}

/////////////////////////////////////////////// Defination of Vulkan related functions //////////////////////////////////////////////



VkResult fillInstanceExtensionNames(void)
{
	// Variable declarations
	VkResult vkresult = VK_SUCCESS;

	// step 1 ->  Find how many instance extension are supported by the vulkun driver for this version and keep it in a local variable.
	uint32_t intanceExtensionCount = 0;

	vkresult = vkEnumerateInstanceExtensionProperties(NULL,&intanceExtensionCount,NULL);
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> First call to vkEnumerateInstanceExtensionProperties() failed\n");
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> First call to vkEnumerateInstanceExtensionProperties() completed succesfully\n");
	}

	// step 2 -> Allocate and Fill struct VkExtensionProperties array corresponding to abhove count.
	VkExtensionProperties*  vkExtensionProperties_array = NULL;

	vkExtensionProperties_array = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties)*intanceExtensionCount); // Here malloc error checking should be done

	vkresult = vkEnumerateInstanceExtensionProperties(NULL,&intanceExtensionCount,vkExtensionProperties_array);
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> Second call to vkEnumerateInstanceExtensionProperties() failed\n");
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> Second call to vkEnumerateInstanceExtensionProperties() completed succesfully\n");
	}

	// step 3 -> Fill and display a local string array of extensions names obtained from VkExtensionProperties structure array.
	char** intanceExtensionNames_array = NULL;

	intanceExtensionNames_array = (char**)malloc(sizeof(char*)*intanceExtensionCount);

	for(uint32_t i = 0;i<intanceExtensionCount;i++)
	{
		intanceExtensionNames_array[i] = (char*)malloc(sizeof(char)*strlen(vkExtensionProperties_array[i].extensionName)+1);
		memcpy(intanceExtensionNames_array[i],vkExtensionProperties_array[i].extensionName,strlen(vkExtensionProperties_array[i].extensionName)+1);
		fprintf(gpFile,"fillInstanceExtensionNames() -> Vulkan extension names = %s\n",intanceExtensionNames_array[i]);
	}

	// step 4 -> As not required here onward free VkExtension Properties array.
	free(vkExtensionProperties_array);
	vkExtensionProperties_array = NULL;

	// step 5 -> Find weather above extension names content our required two extensions ie:
	//		 VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME.
	//           Accordingly set two global variable a: required extension count , b: required extension name array.
	VkBool32 vulkanSurfaceExtensionFound = VK_FALSE;
	VkBool32 win32SurfaceExtensionFound = VK_FALSE;

	for(uint32_t i = 0;i<intanceExtensionCount;i++)
	{
		if(strcmp(intanceExtensionNames_array[i],VK_KHR_SURFACE_EXTENSION_NAME)==0)
		{
			vulkanSurfaceExtensionFound = VK_TRUE;
			enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
		}

		if(strcmp(intanceExtensionNames_array[i],VK_KHR_WIN32_SURFACE_EXTENSION_NAME)==0)
		{
			win32SurfaceExtensionFound = VK_TRUE;
			enabledInstanceExtensionNames_array[enabledInstanceExtensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
		}
	}

	// step 6 -> As not needed henceforth free the local string array
	for(uint32_t i = 0;i<intanceExtensionCount;i++)
	{
		free(intanceExtensionNames_array[i]);
	}

	free(intanceExtensionNames_array);

	// step 7 -> Print weather our vulkun driver supports our required names or not.
	if(vulkanSurfaceExtensionFound == VK_FALSE)
	{
		vkresult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
		fprintf(gpFile, "fillInstanceExtensionNames() -> VK_KHR_SURFACE_EXTENSION_NAME extension not found\n");
		return(vkresult);
	}
	else
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> VK_KHR_SURFACE_EXTENSION_NAME extension found\n");

	}

	if(win32SurfaceExtensionFound == VK_FALSE)
	{
		vkresult = VK_ERROR_INITIALIZATION_FAILED; // return hardcoded failure
		fprintf(gpFile, "fillInstanceExtensionNames() -> VK_KHR_WIN32_SURFACE_EXTENSION_NAME extension not found\n");
		return(vkresult);
	}
	else
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> VK_KHR_WIN32_SURFACE_EXTENSION_NAME extension found\n");

	}

	// step 8 -> Print only enabled supported extensions names
	for(uint32_t i = 0;i<enabledInstanceExtensionCount;i++)
	{
		fprintf(gpFile, "fillInstanceExtensionNames() -> enabled VulkanExtensionName = %s\n",enabledInstanceExtensionNames_array[i]);
	}

	return vkresult;

}


VkResult createVulkanInstance(void)
{
	// Function declaration
	VkResult fillInstanceExtensionNames(void);

	// Variable declarations
	VkResult vkresult = VK_SUCCESS;

	// code

	// step 1 ->  As explaned before fill and initialize required extension names and count in global variable.
	vkresult = fillInstanceExtensionNames();
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "createVulkanInstance() -> fillInstanceExtensionNames() failed\n");
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "createVulkanInstance() -> fillInstanceExtensionNames() completed succesfully\n");
	}

	// step 2 ->  Initialize struct VkApplicationInfo.
	VkApplicationInfo vkApplicationInfo;
	memset((void*)&vkApplicationInfo,0,sizeof(VkApplicationInfo));

	vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkApplicationInfo.pNext = NULL;
	vkApplicationInfo.pApplicationName = gpszAppName;
	vkApplicationInfo.applicationVersion = 1;
	vkApplicationInfo.pEngineName = gpszAppName;
	vkApplicationInfo.engineVersion = 1;
	vkApplicationInfo.apiVersion = VK_API_VERSION_1_4;

	// step 3 -> Initialize struct VkInstanceCreateInfo by using information from step 1 and step 2.
	VkInstanceCreateInfo vkInstanceCreateInfo;
	memset((void*)&vkInstanceCreateInfo,0,sizeof(VkInstanceCreateInfo));

	vkInstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkInstanceCreateInfo.pNext = NULL;
	vkInstanceCreateInfo.pApplicationInfo = &vkApplicationInfo;
	vkInstanceCreateInfo.enabledExtensionCount = enabledInstanceExtensionCount;
	vkInstanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensionNames_array;

	// step 4 ->  Call vkCreateInstance function to get VkInstance in a global variable and do error checking.
	vkresult = vkCreateInstance(&vkInstanceCreateInfo,NULL,&vkInstance);
	if(vkresult == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		fprintf(gpFile, "createVulkanInstance() -> vkCreateInstance() failed due to incompatible driver %d\n",vkresult);
		return(vkresult);
	}
	else if(vkresult == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		fprintf(gpFile, "createVulkanInstance() -> vkCreateInstance() failed due to required extension not present %d\n",vkresult);
		return(vkresult);
	}
	else if(vkresult!=VK_SUCCESS)
	{
		fprintf(gpFile, "createVulkanInstance() -> vkCreateInstance() failed due to unknown reason%d\n",vkresult);
		return(vkresult);
	}
	else
	{
		fprintf(gpFile, "createVulkanInstance() -> vkCreateInstance() Succeded\n");

	}

	return vkresult;

	// step 5 -> Destroy VkInstance in uninitialize function and abhove 4 step in initialize

}



VkResult getSupportedSurface(void)
{
	// Variable declarations
	VkResult vkresult = VK_SUCCESS;

	// 1. Declare a global varibale to hold presenatation surface object.

	// 2. Declare and memset a platfrom specific(Windows,Linux,Android,etc) surfacecreateinfo structure.
	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfoKHR;
	memset((void*)&vkWin32SurfaceCreateInfoKHR,0,sizeof(VkWin32SurfaceCreateInfoKHR));

	// 3. Intialize particularly its hinstance and hwnd members.
	vkWin32SurfaceCreateInfoKHR.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceCreateInfoKHR.pNext = NULL;
	vkWin32SurfaceCreateInfoKHR.flags = 0;
	vkWin32SurfaceCreateInfoKHR.hinstance = (HINSTANCE)GetWindowLongPtr(ghwnd,GWLP_HINSTANCE);// this member can be intialized by getmodule() win32 api also
	vkWin32SurfaceCreateInfoKHR.hwnd = ghwnd;

	// 4. Now call vkCreateWin32SurfaceKHR() to create the presenatation surface object.
	vkresult = vkCreateWin32SurfaceKHR(vkInstance,&vkWin32SurfaceCreateInfoKHR,NULL,&vkSurfaceKHR);
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "getSupportedSurface() -> vkCreateWin32SurfaceKHR() failed\n");
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "getSupportedSurface() -> vkCreateWin32SurfaceKHR() completed succesfully\n");
	}

	return vkresult;

	// 5. Unitialize (vkDestroySurfaceKHR() is generic not platfrom specific)


}


VkResult getPhysicalDevice(void)
{
	// Variable declarations
	VkResult vkresult = VK_SUCCESS;

	// 1. Declare 3 global variables , one for slected physical device, one for slected quque family index , one for physical device memory properties
	//	(required later)

	// code

	uint32_t physicalDeviceCount = 0;

	// 2. Call vkEnummeratePhysicalDevices() function to get the physical device count.
	vkresult = vkEnumeratePhysicalDevices(vkInstance,&physicalDeviceCount,NULL);
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "getPhysicalDevice() -> vkEnumeratePhysicalDevices() first call failed %d\n",vkresult);
		return(vkresult);

	}
	else if(physicalDeviceCount == 0)
	{
		fprintf(gpFile, "getPhysicalDevice() -> vkEnumeratePhysicalDevices() first call failed resultes in physicalDeviceCount is zero\n");
		return(vkresult);
	}
	else
	{
		fprintf(gpFile, "getPhysicalDevice() -> vkEnumeratePhysicalDevices() first call completed succesfully\n");
	}

	// 3. Allocate VkPhyscicalDeviceArray according to abhove count.
	VkPhysicalDevice *vkPhysicalDevice_Array = NULL;

	vkPhysicalDevice_Array = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice)*physicalDeviceCount);

	// 4. Call vkEnumeratePhysicalDevices() again to fill abhove array.
	vkresult = vkEnumeratePhysicalDevices(vkInstance,&physicalDeviceCount,vkPhysicalDevice_Array);
	if (vkresult != VK_SUCCESS)
	{
		fprintf(gpFile, "getPhysicalDevice() -> vkEnumeratePhysicalDevices() second call failed %d\n",vkresult);
		return(vkresult);

	}
	else
	{
		fprintf(gpFile, "getPhysicalDevice() -> vkEnumeratePhysicalDevices() second call completed succesfully\n");
	}

	// 5. Start a loop using abhove physical device count and physical device array.
	//    Note : Declare a boolean bFound variable before this loop which will decide weather we found the desired 
	//    physical device or not,inside this loop 
	VkBool32 bFound = VK_FALSE;

	for(uint32_t i = 0;i<physicalDeviceCount;i++)
	{
		// a: Declare a local variable to hold queue count.
		uint32_t queueCount = UINT32_MAX;

		// b: Call vkGetPhysicalDeviceQueueFamilyProperties() to intialize abhove quque count variable.
		// if physical device is present than it must support atlest one queue family properties. thats why no error checking of this function 
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_Array[i],&queueCount,NULL);

		// c: Allocate VkQueueFamilyPropertiesArray according to abhove count.
		VkQueueFamilyProperties *vkQueueFamilyProperties_Array = NULL;

		vkQueueFamilyProperties_Array = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueCount);

		// d: Call vkGetPhysicalDeviceQueueFamilyProperties() again to fill abhove array.
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_Array[i],&queueCount,vkQueueFamilyProperties_Array);

		// e: Declare VkBool32Array and allocate it using the same abhove queue count.
		VkBool32 *isQueueSurfaceSupported_Array = NULL;

		isQueueSurfaceSupported_Array = (VkBool32*)malloc(sizeof(VkBool32)*queueCount);

		// f: Start a nested loop and fill abhove VkBool32Array by calling vkGetPhysicalDeviceSurfaceSupportKHR().
		for(uint32_t j = 0;j<queueCount;j++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice_Array[i],j,vkSurfaceKHR,&isQueueSurfaceSupported_Array[j]);
		}

		// g: Start another nested loop(not nested inside abhove loop but nested in main loop) and check weather 
		//	   physical device in its array with its queue family has graphics bit or not. if yes then this is a selected 
		//	   physical device and assign it to global variable, simlarly this index is the selected queue family index and 
		//	   assign it to the global variable too, set bFound is equal to true and break out from the second nested loop.
		for(uint32_t j = 0;j<queueCount;j++)
		{
			if(vkQueueFamilyProperties_Array[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				if(isQueueSurfaceSupported_Array[j] == VK_TRUE)
				{
					vkPhysicalDevice_Selected = vkPhysicalDevice_Array[i];
					graphicsQueueFamilyIndex_Selected = j;
					bFound = VK_TRUE;
					break;
				}
			}
		}

		// h: Now we are back in main loop , so free the two array ie:queue family array and the VkBool32Array
		if(isQueueSurfaceSupported_Array)
		{
			free(isQueueSurfaceSupported_Array);
			isQueueSurfaceSupported_Array = NULL;
			fprintf(gpFile, "getPhysicalDevice() -> is sucessed to free isQueueSurfaceSupported_Array\n");
		}

		if(vkQueueFamilyProperties_Array)
		{
			free(vkQueueFamilyProperties_Array);
			vkQueueFamilyProperties_Array = NULL;
			fprintf(gpFile, "getPhysicalDevice() -> is sucessed to free vkQueueFamilyProperties_Array\n");
		}

		// i: Still being in main loop, according to the bFound variable break out from main loop.
		if(bFound == VK_TRUE)
		{
			break;
		}
	}

	// 6. Do error checking according to the value of bFound.
	if(bFound == VK_TRUE)
	{
		fprintf(gpFile, "getPhysicalDevice() -> is sucessed to select required physical device with graphics supported\n");
		if(vkPhysicalDevice_Array)
		{
			// j: Free Physical Device array
			free(vkPhysicalDevice_Array);
			vkPhysicalDevice_Array = NULL;
			fprintf(gpFile, "getPhysicalDevice() -> is sucessed to free vkPhysicalDevice_Array\n");
		}
	}
	else
	{
		fprintf(gpFile, "getPhysicalDevice() -> is failed  to select required physical device with graphics supported\n");
		vkresult = VK_ERROR_INITIALIZATION_FAILED;
		return vkresult;
	}

	// 7. Memset the global physical device memory properties structure.
	memset((void*)&vkPhysicalDeviceMemoryProperties,0,sizeof(VkPhysicalDeviceMemoryProperties));

	// 8. Intialize abhove structure with vkGetPhysicalDeviceMemoryProperties() function.
	vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice_Selected,&vkPhysicalDeviceMemoryProperties);

	// 9. Declare a local structure varaible VkPhysicalDeviceFeatures, memset it and intialize it by calling
	//   vkGetPhyscialDeviceFeature() function.
	VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;

	memset((void*)&vkPhysicalDeviceFeatures,0,sizeof(VkPhysicalDeviceFeatures));

	vkGetPhysicalDeviceFeatures(vkPhysicalDevice_Selected,&vkPhysicalDeviceFeatures);

	// 10. By using "tessilation shader member" of abhove structure check tessilation support
	if(vkPhysicalDeviceFeatures.tessellationShader)
	{
		fprintf(gpFile, "getPhysicalDevice() -> slecteced physical supports tessellationShader\n");
	}
	else
	{
		fprintf(gpFile, "getPhysicalDevice() -> slecteced physical not supports tessellationShader\n");
	}

	// 11 . By using "geometry shader member" of abhove structure check geometry support
	if(vkPhysicalDeviceFeatures.geometryShader)
	{
		fprintf(gpFile, "getPhysicalDevice() -> slecteced physical supports geometryShader\n");
	}
	else
	{
		fprintf(gpFile, "getPhysicalDevice() -> slecteced physical not supports geometryShader\n");
	}

	return vkresult;

}

