import { useEffect, useState } from "react";
import Container from "@mui/material/Container";
import Typography from "@mui/material/Typography";
import Box from "@mui/material/Box";
import Button from "@mui/material/Button";
import Snackbar from "@mui/material/Snackbar";

const deviceNamePrefix = "CYD";
const bleService = "19b10000-e8f2-537e-4f6c-d104768a1214";
const sensorCharacteristic = "19b10001-e8f2-537e-4f6c-d104768a1214";

function App() {
  const isBluetoothAvailable = "bluetooth" in navigator;
  const [errorMessage, setErrorMessage] = useState("");
  const [isConnected, setIsConnected] = useState(false);
  const [blueToothDevice, setBlueToothDevice] = useState<BluetoothDevice>();

  useEffect(() => {
    console.log("Has bluetooth " + isBluetoothAvailable);
  }, []);

  function onDeviceDisconnected() {
    setIsConnected(false);
  }

  async function disconnectDevice(): Promise<void> {
    console.log("Disconnecting", blueToothDevice);
    if (blueToothDevice && blueToothDevice.gatt) {
      blueToothDevice.gatt.disconnect();
    }
  }

  async function connectDevice(): Promise<void> {
    try {
      if (!isBluetoothAvailable) {
        throw new Error("Bluetooth not available");
      }
      const newBlueTooth = await navigator.bluetooth.requestDevice({
        filters: [{ namePrefix: deviceNamePrefix }],
        optionalServices: [bleService],
      });

      if (!newBlueTooth || !newBlueTooth.gatt) {
        throw new Error("Missing something");
      }

      newBlueTooth.addEventListener("gattserverdisconnected", () => {
        onDeviceDisconnected()
      });

      const bleServer = await newBlueTooth.gatt.connect();
      const service = await bleServer.getPrimaryService(bleService);
      const characteristic = await service.getCharacteristic(
        sensorCharacteristic
      );
      characteristic.addEventListener("characteristicvaluechanged", (e) => {
        console.log("Value changed", e);
      });
      characteristic.startNotifications();
      console.log("Notifications Started.");
      const data = await characteristic.readValue();
      console.log("Value", data);
      setIsConnected(true);
      setBlueToothDevice(newBlueTooth);
    } catch (e) {
      console.error(e);
      setErrorMessage("Bluetooth error");
    }
  }

  return (
    <Container maxWidth="sm">
      <Box sx={{ my: 4 }}>
        <Typography variant="h4" component="h1">
          CYD Current Metter
        </Typography>
        <Button
          variant="outlined"
          onClick={() => (isConnected ? disconnectDevice() : connectDevice())}
          sx={{ mr: 2 }}
        >
          {isConnected ? "Disconnect" : "Connect"}
        </Button>
      </Box>
      <Snackbar
        open={errorMessage !== ""}
        autoHideDuration={2000}
        onClose={() => setErrorMessage("")}
        message={errorMessage}
      />
    </Container>
  );
}

export default App;
