import requests
from datetime import datetime


def get_weather(city, weather_api_key):
    city = city.strip().lower()
    url = f"http://api.openweathermap.org/data/2.5/weather"
    params = {
        'q': city,
        'appid': weather_api_key,
        'units': 'metric'
    }
    response = requests.get(url, params=params)

    if response.status_code == 200:
        weather_data = response.json()

        name = weather_data["name"]
        lon = weather_data["coord"]["lon"]
        lat = weather_data["coord"]["lat"]

        url2 = f"http://api.openweathermap.org/data/2.5/forecast?lat={lat}&lon={lon}&appid={weather_api_key}&units=metric"
        forecast_response = requests.get(url2)

        if forecast_response.status_code == 200:
            forecast_data = forecast_response.json()
            forecast_list = forecast_data["list"]

            grouped_forecasts = {}

            for forecast in forecast_list:
                dt_txt = forecast["dt_txt"]
                forecast_date = datetime.strptime(dt_txt, "%Y-%m-%d %H:%M:%S")
                day_of_week = forecast_date.strftime('%A')

                forecast_entry = {
                    'date': forecast_date.strftime('%d %b %H:%M'),
                    'temperature': forecast["main"]['temp'],
                    'description': forecast["weather"][0]["description"],
                    'icon': forecast["weather"][0]["icon"]
                }

                if day_of_week not in grouped_forecasts:
                    grouped_forecasts[day_of_week] = []
                grouped_forecasts[day_of_week].append(forecast_entry)

            return name, grouped_forecasts

    return None, None
            