import os
from flask import Flask, render_template, request
from weather_module.weather import get_weather
from news_module.news import get_news
# import requests
from dotenv import load_dotenv
# from newsapi import NewsApiClient

app = Flask(__name__)

load_dotenv()

weather_api_key = os.getenv('WEATHER_API_KEY')
news_api_key = os.getenv('NEWS_API_KEY')


# def get_weather(city):
#     city = city.strip().lower()
#     url = f"http://api.openweathermap.org/data/2.5/weather"
#     params = {
#         'q': city,
#         'appid': api_key,
#         'units': 'metric'
#     }
#     response = requests.get(url, params=params)
#     print(response)
#     if response.status_code == 200:
#         weather_data = response.json()
#
#         name = weather_data["name"]
#         lon = weather_data["coord"]["lon"]
#         lat = weather_data["coord"]["lat"]
#
#         url2 = f"http://api.openweathermap.org/data/2.5/forecast?lat={lat}&lon={lon}&appid={api_key}&units=metric"
#         forecast_response = requests.get(url2)
#
#         if forecast_response.status_code == 200:
#             forecast_data = forecast_response.json()
#             forecast_message = []
#
#             forecast_list = forecast_data["list"]
#             for forecast in forecast_list:
#                 dt_txt = forecast["dt_txt"]
#                 main = forecast["main"]
#                 description = forecast["weather"][0]["description"]
#                 temp = main['temp']
#                 icon_code = forecast["weather"][0]["icon"]
#
#                 forecast_message.append({
#                     'date': dt_txt,
#                     'temperature': temp,
#                     'description': description,
#                     'icon': icon_code
#                 })
#             return name, forecast_message
#         return None, None
#
#

@app.route('/')
def home():
    return render_template('home.html')


@app.route('/index', methods=['GET', 'POST'])
def index():
    weather_data = None
    city = None
    news = get_news(news_api_key)
    if request.method == 'POST':
        city = request.form.get('city')
        city, weather_data = get_weather(city, weather_api_key)

    return render_template('index.html', weather_data=weather_data, city=city, news=news)


@app.route('/about')
def about_us():
    return render_template('about us.html')


@app.route('/contacts')
def contacts():
    return render_template('contacts.html')


if __name__ == '__main__':
    app.run(debug=True, port=8080)
