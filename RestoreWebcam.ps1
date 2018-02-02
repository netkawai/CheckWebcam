# reset(enable-disable USB driver) until webcam respond.....
do
{
	Get-Device | Where-Object -Property Name -Like '*USB Enhanced Host Controller * 1E2D*' | Disable-Device
	Get-Device | Where-Object -Property Name -Like '*USB Enhanced Host Controller * 1E2D*' | Enable-Device
	PS C:\Users\kawai\Desktop\DeviceManagement\Release> C:\CheckWebcam\CheckWebcam.exe
}while($LastExitCode -ne 0)

