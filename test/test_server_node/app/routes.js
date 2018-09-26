const express = require('express');
const simpleAuth = require('./simpleAuth');
const uuid = require('uuid/v1');
let userRouter = express.Router();

// Routes
userRouter.post('/Users', (req, res) => {
    console.log("POST create" + " " + new Date());
    console.log(req.body);
    let resp = {"id": uuid()};
    res.status(201).json(resp);
    console.log("--------------------------------------------");
    console.log("-------------------Done---------------------");
});
userRouter.put('/Users/:id', (req, res) => {
    console.log("PUT update a user: ", req.params['id'] + " " + new Date());
    console.log(req.body);
    res.status(200).send();
    console.log("--------------------------------------------");
    console.log("-------------------Done---------------------");
});

// get by db id
userRouter.delete('/Users/:id', (req, res) => {
    console.log("DELETE one member by id:" + req.params['id'] + " " + new Date());
    res.sendStatus(204);
    console.log("--------------------------------------------");
    console.log("-------------------Done---------------------");
});

userRouter.get('/Users', (req, res) => {
    res.status(200).send("jodu " + new Date());
});

module.exports = userRouter;
