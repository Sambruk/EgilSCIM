let express = require('express');
let appConfig = require('./config');
let uuid = require('uuid/v1');
let util = require('util');
let log = require('./message_log');

let router = express.Router();

// Routes
router.post('/', (req, res) => {
	console.log("POST create " + req.baseUrl + " " + new Date());
	log("out/"+req.baseUrl+".log", util.inspect(req.body, appConfig.jsonFormat));
	let resp = {"id": uuid()};
	res.status(201).json(resp);
});

router.put('/', (req, res) => {
	console.log("PUT update an " + req.baseUrl + ": " + new Date());
	log("out/"+ req.baseUrl +".log", util.inspect(req.body, appConfig.jsonFormat));
	res.status(200).send();
});

// get by db id
router.delete('/:id', (req, res) => {
	console.log("DELETE one " + req.baseUrl + " by id: " + req.params['id'] + " " + new Date());
	log("out/"+ req.baseUrl +".log", "deleted: " + util.inspect(req.params['id'], appConfig.jsonFormat));
	res.sendStatus(204);
});

router.get('/', (req, res) => {
	res.status(200).send("jodu " + new Date());
});

module.exports = router;
