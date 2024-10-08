import requests


def get_weather(city, weather_api_key):
    city = city.strip().lower()
    url = f"http://api.openweathermap.org/data/2.5/weather"
    params = {
        'q': city,
        'appid': weather_api_key,
        'units': 'metric'
    }
    response = requests.get(url, params=params)
    # print(response)
    if response.status_code == 200:
        weather_data = response.json()

        name = weather_data["name"]
        lon = weather_data["coord"]["lon"]
        lat = weather_data["coord"]["lat"]

        url2 = (f"http://api.openweathermap.org/data/2.5/forecast?lat={lat}&lon={lon}&appid={weather_api_key}&units"
                f"=metric")
        forecast_response = requests.get(url2)

        if forecast_response.status_code == 200:
            forecast_data = forecast_response.json()
            forecast_message = []

            forecast_list = forecast_data["list"]
            # print(forecast_list)
            for forecast in forecast_list:
                dt_txt = forecast["dt_txt"]
                main = forecast["main"]
                description = forecast["weather"][0]["description"]
                temp = main['temp']
                icon_code = forecast["weather"][0]["icon"]

                forecast_message.append({
                    'date': dt_txt,
                    'temperature': temp,
                    'description': description,
                    'icon': icon_code
                })
            return name, forecast_message
        return None, None



