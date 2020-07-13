FROM node:14.4.0-alpine

WORKDIR /app
COPY . .

RUN npm install --production

EXPOSE 3000

CMD ["npm", "start"]
