#!/bin/zsh

urls=(
    "https://cdn.britannica.com/82/94382-050-20CF23DB/Great-Wall-of-China-Beijing.jpg"
    "https://cdn.britannica.com/49/61349-050-9FFBEB28/El-Castillo-pyramid-plaza-Toltec-state-Yucatan.jpg"
    "https://cdn.britannica.com/25/153525-050-FC43840D/Khaznah-Petra-Jordan.jpg"
    "https://cdn.britannica.com/30/94530-050-99493FEA/Machu-Picchu.jpg"
    "https://cdn.britannica.com/54/150754-050-5B93A950/statue-Christ-the-Redeemer-Rio-de-Janeiro.jpg"
    "https://cdn.britannica.com/36/162636-050-932C5D49/Colosseum-Rome-Italy.jpg"
    "https://cdn.britannica.com/86/170586-050-AB7FEFAE/Taj-Mahal-Agra-India.jpg"
)
idx=$(shuf -i 1-7 -n 1)
echo -n "HTTP/1.1 200 OK\r\n"
echo -n "Content-Type: image/jpg\r\n\r\n"
curl --output - ${urls[idx]}
