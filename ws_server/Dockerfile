FROM node:14.4.0-alpine

WORKDIR /app
COPY . .

RUN npm install --production

EXPOSE 33250

CMD ["npm", "start"]
