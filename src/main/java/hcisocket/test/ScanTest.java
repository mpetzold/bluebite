package hcisocket.test;

import org.osgi.service.log.LogService;

import hcisocket.BluetoothHciSocket;
import hcisocket.Buffer;
import hcisocket.DataHandler;
import hcisocket.ErrorHandler;

public class ScanTest {

	private final static int HCI_COMMAND_PKT = 0x01;
	private final static int HCI_ACLDATA_PKT = 0x02;
	private final static int HCI_EVENT_PKT = 0x04;

	private final static int EVT_CMD_COMPLETE = 0x0e;
	private final static int EVT_CMD_STATUS = 0x0f;
	private final static int EVT_LE_META_EVENT = 0x3e;

	private final static int EVT_LE_ADVERTISING_REPORT = 0x02;

	private final static int OGF_LE_CTL = 0x08;
	private final static int OCF_LE_SET_SCAN_PARAMETERS = 0x000b;
	private final static int OCF_LE_SET_SCAN_ENABLE = 0x000c;

	private final static int LE_SET_SCAN_PARAMETERS_CMD = OCF_LE_SET_SCAN_PARAMETERS | OGF_LE_CTL << 10;
	private final static int LE_SET_SCAN_ENABLE_CMD = OCF_LE_SET_SCAN_ENABLE | OGF_LE_CTL << 10;

	private final static int HCI_SUCCESS = 0;
	
	private static final String[] GAP_ADV_TYPES = {"ADV_IND", "ADV_DIRECT_IND", "ADV_SCAN_IND", "ADV_NONCONN_IND", "SCAN_RSP"};
	private static final String[] GAP_ADDR_TYPES = {"PUBLIC", "RANDOM"};
	
	private final BluetoothHciSocket bluetoothHCISocket;
	
	public ScanTest(final LogService logService, BluetoothHciSocket bluetoothHCISocket) {
		this.bluetoothHCISocket = bluetoothHCISocket;
		this.bluetoothHCISocket.onData(new DataHandler() {
			public void handleData(Buffer data) {
				
				logService.log(LogService.LOG_INFO, "DATA: " + data.toString("hex"));

				if (data.readUInt8(0) == HCI_EVENT_PKT) {
					if (data.readUInt8(1) == EVT_CMD_COMPLETE) {
						if (data.readUInt16LE(4) == LE_SET_SCAN_PARAMETERS_CMD) {
							if (data.readUInt8(6) == HCI_SUCCESS) {
								logService.log(LogService.LOG_INFO, "LE Scan Parameters Set");
							}
						} else if (data.readUInt16LE(4) == LE_SET_SCAN_ENABLE_CMD) {
							if (data.readUInt8(6) == HCI_SUCCESS) {
								logService.log(LogService.LOG_INFO, "LE Scan Enable Set");
							}
						}
					} else if (data.readUInt8(1) == EVT_LE_META_EVENT) {
						if (data.readUInt8(3) == EVT_LE_ADVERTISING_REPORT) { // subevent
							int gapAdvType = data.readUInt8(5);
							int gapAddrType = data.readUInt8(6);
							Buffer gapAddr = data.slice(7, 13);
							
							Buffer eir = data.slice(14, data.length - 1);
							int rssi = data.readInt8(data.length - 1);
							
							logService.log(LogService.LOG_INFO, "LE Advertising Report");
							logService.log(LogService.LOG_INFO, "\t" + GAP_ADV_TYPES[gapAdvType]);
							logService.log(LogService.LOG_INFO, "\t" + GAP_ADDR_TYPES[gapAddrType]);
							logService.log(LogService.LOG_INFO, "\t" + gapAddr.toString("hex")/*.match(/.{1,2}/g).reverse().join(':')*/);
							logService.log(LogService.LOG_INFO, "\t" + eir.toString("hex"));
							logService.log(LogService.LOG_INFO, "\t" + rssi);
						}
					}
				}
			}
		});
		this.bluetoothHCISocket.onError(new ErrorHandler() {
			public void handleError(String message) {
				logService.log(LogService.LOG_ERROR, message);
			}
		});
		
		this.setFilter();
		this.bluetoothHCISocket.start();
		
		this.setScanEnable(false, true);
		this.setScanParameters();
		this.setScanEnable(true, true);
	}
	
	private void setFilter() {
		Buffer filter = new Buffer(14);
		int typeMask = (1 << HCI_EVENT_PKT);
		int eventMask1 = (1 << EVT_CMD_COMPLETE) | (1 << EVT_CMD_STATUS);
		int eventMask2 = (1 << (EVT_LE_META_EVENT - 32));
		int opcode = 0;
		
		filter.writeUInt32LE(typeMask, 0);
		filter.writeUInt32LE(eventMask1, 4);
		filter.writeUInt32LE(eventMask2, 8);
		filter.writeUInt16LE(opcode, 12);
		  
		this.bluetoothHCISocket.setFilter(filter.bytes);
	}
	
	private void setScanParameters() {
		Buffer cmd = new Buffer(11);
		
		// header
		cmd.writeUInt8(HCI_COMMAND_PKT, 0);
		cmd.writeUInt16LE(LE_SET_SCAN_PARAMETERS_CMD, 1);
		
		// length
		cmd.writeUInt8(0x07, 3);
		
		// data
		cmd.writeUInt8(0x01, 4); // type: 0 -> passive, 1 -> active
		cmd.writeUInt16LE(0x0010, 5); // internal, ms * 1.6
		cmd.writeUInt16LE(0x0010, 7); // window, ms * 1.6
		cmd.writeUInt8(0x00, 9); // own address type: 0 -> public, 1 -> random
		cmd.writeUInt8(0x00, 10); // filter: 0 -> all event types
		
		this.bluetoothHCISocket.write(cmd.bytes);
	}
	
	private void setScanEnable(boolean enabled, boolean duplicates) {
		Buffer cmd = new Buffer(6);
		
		// header
		cmd.writeUInt8(HCI_COMMAND_PKT, 0);
		cmd.writeUInt16LE(LE_SET_SCAN_ENABLE_CMD, 1);
		
		// length
		cmd.writeUInt8(0x02, 3);
		
		// data
		cmd.writeUInt8(enabled ? 0x01 : 0x00, 4); // enable: 0 -> disabled, 1 -> enabled
		cmd.writeUInt8(duplicates ? 0x01 : 0x00, 5); // duplicates: 0 -> no duplicates, 1 -> duplicates
		
		this.bluetoothHCISocket.write(cmd.bytes);
	}
	
}
