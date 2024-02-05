import { useCallback, useEffect, useState } from "react";
import Container from "@mui/material/Container";
import Typography from "@mui/material/Typography";
import Button from "@mui/material/Button";
import Snackbar from "@mui/material/Snackbar";

import Grid from "@mui/material/Grid";
import Chart from "react-apexcharts";
import { ApexOptions } from "apexcharts";

const deviceNamePrefix = "CYD";
const bleServiceUUID = "19b10000-e8f2-537e-4f6c-d104768a1214";
const sensorCharacteristicUUID = "19b10001-e8f2-537e-4f6c-d104768a1214";
//
// get everything back until time is different
const chartOptions: ApexOptions = {
  stroke: { width: 1, curve: "smooth" },
  chart: {

    background: 'transparent',
    id: "miliAmpsChart",
    animations: {
      enabled: true,
      easing: "linear",
      dynamicAnimation: {
        speed: 200,
      },
    },
    toolbar: {
      export: {
        csv: {
          filename: undefined,
          columnDelimiter: ",",
          headerCategory: "Date",
          headerValue: "miliAmps",
        },
        png: {
          filename: undefined,
        },
      },
    },
    zoom: {
      enabled: true,
      type: "x",
      autoScaleYaxis: true,
      zoomedArea: {
        fill: {
          color: "#90CAF9",
          opacity: 0.4,
        },
        stroke: {
          color: "#0D47A1",
          opacity: 0.4,
          width: 1,
        },
      },
    },
    width: "100%",
    height: 600,
  },
  yaxis: {
    title: {
      text: 'miliAmps'
    },
   // min: 0,
  },

  xaxis: {
    title: {
      text: 'Seconds from start'
    },
    type: "datetime",
    labels: {
      formatter: function (value: string, timestamp: number): string {
        return Math.round(timestamp / 1000) + "s";
      },
    },
  },
  theme: { mode: "dark" },
};

function App() {
  const isBluetoothAvailable = "bluetooth" in navigator;
  const [errorMessage, setErrorMessage] = useState("");
  const [statusText, setStatus] = useState("");
  const [isConnected, setIsConnected] = useState(false);
  const [isBusy, setIsBusy] = useState(false);

  const [series, setSeries] = useState([]);
  const [blueToothDevice, setBlueToothDevice] =
    useState<BluetoothDevice | null>(null);
  const [characteristic, setCharacteristic] =
    useState<BluetoothRemoteGATTCharacteristic | null>(null);

  const onNewValue = useCallback((event: Event) => {
    const characteristic = event.target as BluetoothRemoteGATTCharacteristic;
    const dataView = characteristic.value!;
    const littleEndian = true;
    const time = dataView.getUint32(0, littleEndian);
    const current = dataView.getUint16(4, littleEndian);
    console.log("Time", time, "Current", current);
    setStatus(`Last update: ${current}mA @ ${Math.round(time/1000)}seconds`);
    setSeries((prevSeries) => [...prevSeries, [time, current] as never]);
  }, []);

  const onDeviceDisconnected = useCallback(() => {
    console.log("Device disconnected");
    characteristic?.removeEventListener(
      "characteristicvaluechanged",
      onNewValue as unknown as EventListener
    );
    setCharacteristic(null);
    setIsConnected(false);
  }, [characteristic, onNewValue]);

  useEffect(() => {
    return () => {
      if (characteristic) {
        characteristic.removeEventListener(
          "characteristicvaluechanged",
          onNewValue as unknown as EventListener
        );
        setCharacteristic(null);
      }
    };
  }, [characteristic, onNewValue]);

  useEffect(() => {
    return () => {
      if (blueToothDevice) {
        blueToothDevice.removeEventListener(
          "gattserverdisconnected",
          onDeviceDisconnected as unknown as EventListener
        );
        setBlueToothDevice(null);
      }
    };
  }, [blueToothDevice, onDeviceDisconnected]);

  async function disconnectDevice(): Promise<void> {
    console.log("Disconnecting", blueToothDevice);
    if (blueToothDevice?.gatt?.connected) {
      blueToothDevice.gatt.disconnect();
    }
  }

  async function connectDevice(): Promise<void> {
    setIsBusy(true);
    setSeries([]);
    try {
      if (!isBluetoothAvailable) {
        throw new Error("Bluetooth not available");
      }
      const newBlueTooth = await navigator.bluetooth.requestDevice({
        filters: [{ namePrefix: deviceNamePrefix }],
        optionalServices: [bleServiceUUID],
      });

      if (!newBlueTooth || !newBlueTooth.gatt) {
        throw new Error("Missing something");
      }

      newBlueTooth.addEventListener(
        "gattserverdisconnected",
        onDeviceDisconnected as unknown as EventListener
      );
      const bleServer = await newBlueTooth.gatt.connect();
      const service = await bleServer.getPrimaryService(bleServiceUUID);
      const newCharacteristic = await service.getCharacteristic(
        sensorCharacteristicUUID
      );
      newCharacteristic.addEventListener(
        "characteristicvaluechanged",
        onNewValue as unknown as EventListener
      );
      await newCharacteristic.startNotifications();
      setBlueToothDevice(newBlueTooth);
      setCharacteristic(newCharacteristic);
      setIsConnected(true);
    } catch (e) {
      console.error(e);
      setErrorMessage("Bluetooth error");
    } finally {
      setIsBusy(false);
    }
  }

  return (
    <Container maxWidth="lg">
      <Grid container spacing={2} sx={{ mt: 2 }}>
        <Grid item xs={10}>
          <Typography variant="h4" component="h1">
            CYD Monitor ⚡️ {statusText}
          </Typography>
        </Grid>

        <Grid item xs={2} container justifyContent="flex-end">
          <Button
            disabled={isBusy}
            variant="outlined"
            onClick={() => (isConnected ? disconnectDevice() : connectDevice())}
          >
            {isConnected ? "Disconnect" : "Connect"}
          </Button>
        </Grid>
        <Grid item xs={12}>
          <Chart
            options={chartOptions}
            series={[
              {
                name: "miliAmps",
                data: series,
              },
            ]}
            type="line"
            width="100%"
          />
        </Grid>
      </Grid>

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
