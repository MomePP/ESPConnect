use http_types::headers::HeaderValue;
use tide::log;
use tide::prelude::*;
use tide::security::{CorsMiddleware, Origin};
use tide::Request;
use tide::Response;

#[derive(serde::Deserialize)]
struct ConnectionRequest {
    ssid: String,
    username: String,
    password: String,
}

async fn scan_handler(_req: Request<()>) -> tide::Result {
    let res = Response::builder(tide::StatusCode::Ok)
        .body(json!([
            { "name": "momeppkt", "type": "WPA_WPA2_PSK" },
            { "name": "tlic-test", "type": "OPEN" },
            { "name": "@JumboPlus", "type": "WPA2_ENTERPRISE" },
            { "name": "", "type": "WPA2_PSK", "hidden": true }
        ]))
        .build();
    Ok(res)
}

async fn connect_handler(mut req: Request<()>) -> tide::Result {
    let wifi_requested: ConnectionRequest = req.body_json().await?;
    log::info!(
        "connecting to {}, [user: {}, pwd: {}]",
        wifi_requested.ssid,
        wifi_requested.username,
        wifi_requested.password
    );

    let res = Response::builder(tide::StatusCode::Ok)
        .body(json!({ "status": "ok" }))
        .build();
    Ok(res)
}

#[async_std::main]
async fn main() -> tide::Result<()> {
    femme::start();

    let allowed_origins = vec!["http://localhost:8080", "http://localhost:5173"];
    let cors = CorsMiddleware::new()
        .allow_methods("GET, POST, OPTIONS".parse::<HeaderValue>().unwrap())
        .allow_origin(Origin::from(allowed_origins))
        .allow_credentials(false);
    let logs = tide::log::LogMiddleware::new();

    let mut app = tide::new();
    app.with(cors);
    app.with(logs);

    app.at("/")
        .get(|_| async { Ok("device mockup server running !") });
    app.at("espconnect/scan").get(scan_handler);
    app.at("espconnect/connect").post(connect_handler);

    app.listen("0.0.0.0:9000").await?;
    Ok(())
}

