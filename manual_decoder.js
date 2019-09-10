var decoder = require("./decoder")
var fs = require('fs');
// my code here
function manual_decode(filename) {
    var contents = fs.readFileSync(filename, 'utf8');

    var json_obj = JSON.parse(contents)

    out_array = []
    json_obj.rows.forEach(function (element) {
        // console.log(element);
        port = element.PORT
        hex = Buffer.from(element.data, 'hex');
        decoded_data = decoder(hex)
        // console.log(decoded_data);
        element.decoded_data = decoded_data
        out_array.push(element)
    });

    // console.log(out_array)
    json_obj.rows = out_array
    return json_obj
}



// example
file = "tracker21.json"
arr = manual_decode(file)
console.log(arr)

// save to file
var jsonData = JSON.stringify(arr, null, 2);

fs.writeFile("tracker21_decoded.json", jsonData, function (err) {
    if (err) {
        return console.log(err);
    }

    console.log("The file was saved!");
});

