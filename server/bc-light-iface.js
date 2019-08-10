const SerialPort = require('serialport');
const Readline = SerialPort.parsers.Readline;
const port = new SerialPort('/dev/ttyUSB0', {
    baudRate: 115200,
});
const parser = port.pipe(new Readline({ delimiter: '\r\n' }));
const mqtt = require('mqtt');
const mqtt_set_topic = 'node/loznice/lights/+/set';

let light1 = false;
let light2 = false;
let light3 = false;
let light4 = false;

const client = mqtt.connect('mqtt://host', {
    username: 'username',
    password: 'password',
});

client.on('connect', () => {
    client.subscribe(mqtt_set_topic);
});

const serialWrite = (text) => {
    port.write(text + '\r\n', function (err) {
        if (err) {
            return console.log('Error on write:', err.message);
        }
        console.log('Written:', text);
    });
};

serialWrite('AT$LIGHTS?');

port.on('error', function (err) {
    console.log('Error:', err.message);
});

client.on('message', function (topic, message) {
    const msg = message.toString();
    const re = topic.match(new RegExp(mqtt_set_topic.replace('+', '(\\d+)')));
    if (re && re.length === 2) {
        const num = parseInt(re[1]);
        const parts = [light1, light2, light3, light4];
        parts[num - 1] = msg === 'true';
        parts.forEach((value, index) => {
            parts[index] = value ? '1' : '0';
        });
        serialWrite('AT$LIGHTS=' + parts.join(','));
    }
});

const sendStatus = () => {
    client.publish('node/loznice/lights/1/status', light1.toString());
    client.publish('node/loznice/lights/2/status', light2.toString());
    client.publish('node/loznice/lights/3/status', light3.toString());
    client.publish('node/loznice/lights/4/status', light4.toString());
};
parser.on('data', function (data) {
    console.log('Read:', data);

    if (data.startsWith('$STATUS:')) {
        let values = data.substr(8).split(',');
        light1 = values[0] === '1';
        light2 = values[1] === '1';
        light3 = values[2] === '1';
        light4 = values[3] === '1';

        sendStatus();
    }
});
