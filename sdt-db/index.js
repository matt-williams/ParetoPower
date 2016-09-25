var express = require('express');
var MongoClient = require('mongodb').MongoClient;
var assert = require('assert');
var bodyParser = require('body-parser');

// Connection URL
var url = 'mongodb://localhost:27017/sdt-db';

// Use connect method to connect to the server
var mongodb;
MongoClient.connect(url, function(err, db) {
  assert.equal(null, err);
  console.log("Connected to mongodb");
  mongodb = db;
});

var app = express();
app.use(bodyParser.json());

app.get('/', function (req, res) {
  res.send('Hello World!');
});

app.get('/serviceDeliveryTokens/:key', function (req, res) {
  console.log("Querying for key " + req.params.key);
  var collection = mongodb.collection('serviceDeliveryTokens');
/*
  collection.findById(req.params.key, function (err, record) {
    assert.equal(err, null);
    console.log("Found the following record");
    console.log(record)
    res.send(record);
  });
*/
  collection.find({key: req.params.key}).toArray(function(err, sbts) {
    assert.equal(err, null);
    console.log("Found the following records");
    console.log(sbts)
    res.send(sbts[0]);
  });
});

/*
app.post('/serviceDeliveryTokens/', function (req, res) {
  console.log("Inserting " + JSON.stringify(req.body));
  var collection = mongodb.collection('serviceDeliveryTokens');
  collection.insert(req.body, {"new": true}, function(err, results) {
    assert.equal(err, null);
    res.set('Location', '/serviceDeliveryTokens/' + results.ops[0]._id);
    res.send(results.ops[0]);
  });
});
*/

app.put('/serviceDeliveryTokens/:key', function (req, res) {
  console.log("Updating key " + req.params.key + " with " + JSON.stringify(req.body));
  var collection = mongodb.collection('serviceDeliveryTokens');
  collection.update({key: req.params.key}, {$set: req.body}, {upsert: true}, function(err, results) {
    assert.equal(err, null);
    res.send(results[0]);
  });
});

app.listen(8080, function () {
  console.log('Listening on port 8080');
});
