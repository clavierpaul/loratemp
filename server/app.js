require('dotenv').config()
const mqtt = require("mqtt");

const client = mqtt.connect("mqtts://eu.thethings.network", {
    port: 8883,
    username: "fluxe-test",
    password: process.env.MQTT_PASS 
});

const print_result = (data) => {
    console.log("");
    console.log(`Time: ${data.time}`);
    console.log(`Temperature: ${data.temp}°F`);
}

client.on('connect', () => {
    console.log("Connected to The Things Network");
    client.subscribe("fluxe-test/devices/explorer/up", (err) => {
        if (err) {
            console.log(err.message);
        } else {
            console.log("Subscribed to topic")
        }
    });
})

client.on('message', (message) => {
    const data = JSON.parse(message.toString());

    const time = new Date(data.metadata.time).toLocaleTimeString();
    const temp = data.payload_fields.temperature_3 * 9/5 + 32;

    print_result({time: time, temp: temp});
});