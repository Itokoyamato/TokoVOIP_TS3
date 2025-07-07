FROM node:24-alpine

WORKDIR /app
COPY . .

RUN npm install --production

EXPOSE 3001

CMD ["npm", "start"]