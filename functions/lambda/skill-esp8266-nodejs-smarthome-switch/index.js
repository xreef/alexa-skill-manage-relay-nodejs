'use strict';

// Import required AWS SDK clients and commands for Node.js
const {
    DynamoDB
} = require("@aws-sdk/client-dynamodb");

const configDynamoDB = {
    version: 'latest',
    region: "eu-west-1",
    // endpoint: "http://dynamodb-local:8000",
    // credentials: {
    //     // For security reasons, do not store AWS Credentials in your files. Use Amazon Cognito instead.
    //     accessKeyId: "9oiaf7",
    //     // secretAccessKey default can be used while using the downloadable version of DynamoDB.
    //     // For security reasons, do not store AWS Credentials in your files. Use Amazon Cognito instead.
    //     secretAccessKey: "yz5i9"
    // }
};


let AlexaResponse = require("./alexa/skills/smarthome/AlexaResponse");


exports.handler = async function (request, context) {

    // Dump the request for logging - check the CloudWatch logs
    log("index.handler request  -----");
    log(JSON.stringify(request));

    if (context !== undefined) {
        log("index.handler context  -----");
        log(JSON.stringify(context));
    }

    // Validate we have an Alexa directive
    if (!('directive' in request)) {
        let aer = new AlexaResponse(
            {
                "name": "ErrorResponse",
                "payload": {
                    "type": "INVALID_DIRECTIVE",
                    "message": "Missing key: directive, Is request a valid Alexa directive?"
                }
            });
        return sendResponse(aer.get());
    }

    // Check the payload version
    if (request.directive.header.payloadVersion !== "3") {
        let aer = new AlexaResponse(
            {
                "name": "ErrorResponse",
                "payload": {
                    "type": "INTERNAL_ERROR",
                    "message": "This skill only supports Smart Home API version 3"
                }
            });
        return sendResponse(aer.get())
    }

    let namespace = ((request.directive || {}).header || {}).namespace;

    if (namespace.toLowerCase() === 'alexa.authorization') {
        let aar = new AlexaResponse({"namespace": "Alexa.Authorization", "name": "AcceptGrant.Response",});
        return sendResponse(aar.get());
    }

    if (request.directive.header.namespace === 'Alexa.Discovery' && request.directive.header.name === 'Discover') {
        log("DEBUG:", "Discover request",  JSON.stringify(request));
        handleDiscovery(request, context, "");
    }
    else if (request.directive.header.namespace === 'Alexa.PowerController') {
        if (request.directive.header.name === 'TurnOn' || request.directive.header.name === 'TurnOff') {
            log("DEBUG:", "TurnOn or TurnOff Request", JSON.stringify(request));
            await handlePowerControl(request, context);
        }
    }

    function handleDiscovery(request, context) {
        let adr = new AlexaResponse({"namespace": "Alexa.Discovery", "name": "Discover.Response"});
        let capability_alexa = adr.createPayloadEndpointCapability();
        let capability_alexa_powercontroller = adr.createPayloadEndpointCapability({"interface": "Alexa.PowerController", "supported": [{"name": "powerState"}]});
        adr.addPayloadEndpoint({"friendlyName": "Relay test", "description": "esp8266 relay test www.mischianti.org", "endpointId": "esp8266-relay-01", "capabilities": [capability_alexa, capability_alexa_powercontroller]});

        const payload = sendResponse(adr.get());
        // var header = request.directive.header;
        // header.name = "Discover.Response";
        // log("DEBUG", "Discovery Response: ", JSON.stringify({ header: header, payload: payload }));
        // context.succeed({ request: { header: header, payload: payload } });
        context.succeed(payload);
    }

};

const handlePowerControl =  async (request, context) => {
    var requestMethod = request.directive.header.name;

    let power_state_value = "OFF";
    if (requestMethod === "TurnOn")
        power_state_value = "ON";

    let endpoint_id = request.directive.endpoint.endpointId;
    let token = request.directive.endpoint.scope.token;
    let correlationToken = request.directive.header.correlationToken;

    let ar = new AlexaResponse(
        {
            "correlationToken": correlationToken,
            "token": token,
            "endpointId": endpoint_id
        }
    );
    ar.addContextProperty({"namespace":"Alexa.PowerController", "name": "powerState", "value": power_state_value});

    // Check for an error when setting the state
    let state_set = await sendDeviceState(endpoint_id, "Power", power_state_value);

    if (state_set.$metadata.httpStatusCode != 200) {
        return new AlexaResponse(
            {
                "name": "ErrorResponse",
                "payload": {
                    "type": "ENDPOINT_UNREACHABLE",
                    "message": "Unable to reach endpoint database."
                }
            }).get();
    }

    const response = sendResponse(ar.get());
    context.succeed(response);
}


function sendResponse(response)
{
    // TODO Validate the response
    log("index.handler response -----");
    log(JSON.stringify(response));
    return response
}



const sendDeviceState = (endpoint_id, state, value) => {
    //     let state = "Power";
    //     let value = "On";
    //     let endpoint_id = "<end-points>";

        let key = state + "Value";
        let attribute_obj = {};
        attribute_obj[key] = {"Action": "PUT", "Value": {"S": value}};

        var params = {
            TableName: "SmartHomeEsp8266",
            Key: {"ItemId": {"S": endpoint_id}},
            AttributeUpdates: attribute_obj,
            ReturnValues: "UPDATED_NEW"
        };

        const dbClient = new DynamoDB (configDynamoDB);
        return dbClient.updateItem(params);
}

function log(message) {
    console.log(message);
}
