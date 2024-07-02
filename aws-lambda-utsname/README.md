# aws-lambda-ustname

**aws-lambda-utsname** retrieves and returns **struct utsname** information from Lambda environment.

## Compile

1. `cmake -DCMAKE_BUILD_TYPE=MinSizeRel,RelWithDebInfo .`
2. `make`
3. `make make aws-lambda-package-uname-lambda-function`

## Deploy

`aws lambda create-function --region YOUR_REGION --function-name UnameLambdaFunction --handler uname-lambda-function --runtime provided.al2 --memory-size 128 --timeout 15 --role YOUR_ROLE --zip-file fileb://uname-lambda-function.zip`

## Invoke

`aws lambda invoke --region YOUR_REGION --function-name UnameLambdaFunction /dev/stdout | jq`

```json
{
  "sysname": "Linux",
  "nodename": "169.254.84.29",
  "release": "4.14.255-344-281.563.amzn2.x86_64",
  "version": "#1 SMP Wed May 22 13:14:11 UTC 2024",
  "machine": "x86_64",
  "domainname": "(none)"
}
{
  "StatusCode": 200,
  "ExecutedVersion": "$LATEST"
}
```
