function GetWatchdogComPort() {
     $portList = get-pnpdevice -class Ports -ea 0
     if ($portList) {
          foreach ($device in $portList) {
               if ($device.Present) {
                    if ($device.Name -match '^(.*)\((COM\d+)\)$') {
                         $port = $matches[2]
                         $name = $matches[1].Trim()
                         if ($name -ieq "Arduino Uno") {
                              return $port
                         }
                    }
               }
          }
     }
     return $null
}


$comPort = GetWatchdogComPort
if (!$comPort) {
     Write-Error -Message "Could not find COM Port"
     exit 1
}

try {
     $port = new-Object System.IO.Ports.SerialPort $comPort, 115200, None, 8, one
     $port.Open()
     while ($port.IsOpen) {
          $data = $port.ReadExisting()
          if ($data) {
               Write-Output "${comPort}: ${data}"
          }
          $port.Write("H")
          Start-Sleep -s 5
     }
}
finally {
     $port.Close()
}
