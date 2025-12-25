use reqwest::get;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct IpInfo {
    pub status: String,
    pub continent: String,
    pub continent_code: String,
    pub country: String,
    pub country_code: String,
    pub region: String,
    pub region_name: String,
    pub city: String,
    pub district: String,
    pub zip: String,
    pub lat: f64,
    pub lon: f64,
    pub timezone: String,
    pub offset: i32,
    pub currency: String,
    pub isp: String,
    pub org: String,
    #[serde(rename = "as")]
    pub as_info: String,
    pub asname: String,
    pub reverse: String,
    pub mobile: bool,
    pub proxy: bool,
    pub hosting: bool,
    #[serde(rename = "query")]
    pub ip: String,
}

pub async fn get_ip_info() -> Option<IpInfo> {
    let response = get("http://ip-api.com/json/?fields=66846719").await;

    if response.is_err() {
        return None;
    }

    let body = response.unwrap().text().await;

    if body.is_err() {
        return None;
    }

    let geo_info = body.unwrap();
    let geo_info: IpInfo = serde_json::from_str(&geo_info).ok()?;

    Some(geo_info)
}
