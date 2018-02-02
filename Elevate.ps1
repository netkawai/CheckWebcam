param([String]$execScript='') 
$username = "Administrator" 
$password = "CUqNBfu7"
$startWithElevatedRights = "powershell -ArgumentList '-ExecutionPolicy ByPass -File $execScript' "

$credentials = New-Object System.Management.Automation.PSCredential -ArgumentList @($username,(ConvertTo-SecureString -String $password -AsPlainText -Force))
$ps  = Start-Process -PassThru -FilePath powershell -Credential $credentials -ArgumentList '-noprofile -command &{Start-Process ',  $startWithElevatedRights, ' -Verb runas}'
$ps.WaitForExit()